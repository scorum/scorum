#include <scorum/chain/services/registration_pool.hpp>
#include <scorum/chain/database/database.hpp>

#include <scorum/chain/services/registration_committee.hpp>

namespace scorum {
namespace chain {

dbs_registration_pool::dbs_registration_pool(database& db)
    : _base_type(db)
{
}

const registration_pool_object& dbs_registration_pool::get() const
{
    try
    {
        // This method uses get(id == 0). It is correct because we create object only one time
        return db_impl().get<registration_pool_object>();
    }
    FC_CAPTURE_AND_RETHROW(("Pool does not exist."))
}

bool dbs_registration_pool::is_exists() const
{
    return nullptr != db_impl().find<registration_pool_object>();
}

const registration_pool_object& dbs_registration_pool::create_pool(const asset& supply,
                                                                   const asset& maximum_bonus,
                                                                   const schedule_items_type& schedule_items)
{
    FC_ASSERT(supply > asset(0, SCORUM_SYMBOL), "Registration supply amount must be more than zerro.");
    FC_ASSERT(maximum_bonus > asset(0, SCORUM_SYMBOL), "Registration maximum bonus amount must be more than zerro.");
    FC_ASSERT(!schedule_items.empty(), "Registration schedule must have at least one item.");

    // check schedule
    for (const auto& value : schedule_items)
    {
        const auto& stage = value.first;
        const schedule_item_type& item = value.second;
        FC_ASSERT(item.users > 0, "Invalid schedule value (users in thousands) for stage ${1}.", ("1", stage));
        FC_ASSERT(item.bonus_percent >= 0 && item.bonus_percent <= 100,
                  "Invalid schedule value (percent) for stage ${1}.", ("1", stage));
    }

    // Check existence here to allow unit tests check input data even if object exists in DB
    // This method uses get(id == 0). Look get_pool.
    FC_ASSERT(db_impl().find<registration_pool_object>() == nullptr, "Can't create more than one pool.");

    // create pool
    const auto& new_pool = db_impl().create<registration_pool_object>([&](registration_pool_object& pool) {
        pool.balance = supply;
        pool.maximum_bonus = maximum_bonus;
        pool.schedule_items.reserve(schedule_items.size());
        for (const auto& value : schedule_items)
        {
            pool.schedule_items.push_back(value.second);
        }
    });

    return new_pool;
}

asset dbs_registration_pool::calculate_per_reg()
{
    const registration_pool_object& this_pool = get();

    FC_ASSERT(!this_pool.schedule_items.empty(), "Invalid schedule.");

    // find position in schedule
    std::size_t ci = 0;
    uint64_t allocated_rest = this_pool.already_allocated_count;
    auto it = this_pool.schedule_items.begin();
    for (; it != this_pool.schedule_items.end(); ++it, ++ci)
    {
        uint64_t item_users_limit = (*it).users;

        if (allocated_rest >= item_users_limit)
        {
            allocated_rest -= item_users_limit;
        }
        else
        {
            break;
        }
    }

    if (it == this_pool.schedule_items.end())
    {
        // no more schedule items (out of schedule),
        // use last stage to calculate bonus
        --it;
    }

    using schedule_item_type = registration_pool_object::schedule_item;
    const schedule_item_type& current_item = (*it);
    return asset(current_item.bonus_percent * this_pool.maximum_bonus.amount / 100, SCORUM_SYMBOL);
}

asset dbs_registration_pool::decrease_balance(const asset& balance)
{
    FC_ASSERT(balance.amount > 0, "Invalid balance.");

    const registration_pool_object& this_pool = get();

    asset ret(0, SCORUM_SYMBOL);

    db_impl().modify(this_pool, [&](registration_pool_object& pool) {
        if (pool.balance.amount > 0 && balance <= pool.balance)
        {
            pool.balance -= balance;
            ret = balance;
        }
        else
        {
            ret = pool.balance;
            pool.balance = asset(0, SCORUM_SYMBOL);
        }
    });

    return ret;
}

bool dbs_registration_pool::check_autoclose()
{
    const registration_pool_object& this_pool = get();

    if (this_pool.balance.amount <= 0)
    {
        _close();
        return true;
    }
    else
    {
        return false;
    }
}

void dbs_registration_pool::increase_already_allocated_count()
{
    const registration_pool_object& this_pool = get();

    db_impl().modify(this_pool, [&](registration_pool_object& pool) { pool.already_allocated_count++; });
}

asset dbs_registration_pool::allocate_cash(const account_name_type& member_name)
{
    asset per_reg = calculate_per_reg();
    // clang-format off
    FC_ASSERT(per_reg.amount > 0, "Invalid schedule. Zero bonus return.");
    FC_ASSERT(per_reg <= SCORUM_REGISTRATION_BONUS_LIMIT_PER_MEMBER_PER_N_BLOCK / SCORUM_REGISTRATION_BONUS_LIMIT_PER_MEMBER_N_BLOCK,
              "Invalid schedule. Per registration bonus ${1} always exceeds the limit ${2}.",
              ("1", per_reg)("2",SCORUM_REGISTRATION_BONUS_LIMIT_PER_MEMBER_PER_N_BLOCK / SCORUM_REGISTRATION_BONUS_LIMIT_PER_MEMBER_N_BLOCK));
    // clang-format on

    dbs_registration_committee& committee_service = db().obtain_service<dbs_registration_committee>();

    uint32_t head_block_num = db_impl().head_block_num();

    const registration_committee_member_object& member = committee_service.get_member(member_name);

    uint32_t last_allocated_block = member.last_allocated_block;
    FC_ASSERT(last_allocated_block <= head_block_num);

    if (!last_allocated_block)
    {
        // not any allocation
        last_allocated_block = head_block_num;
    }

    uint32_t pass_blocks = head_block_num - last_allocated_block;

    uint32_t per_n_block_remain = member.per_n_block_remain;
    if (pass_blocks > per_n_block_remain)
    {
        per_n_block_remain = 0;
    }
    else
    {
        per_n_block_remain -= pass_blocks;
    }

    if (per_n_block_remain > 0)
    {
        // check limits
        share_type limit_per_memeber = (share_type)(pass_blocks + 1);
        limit_per_memeber *= SCORUM_REGISTRATION_BONUS_LIMIT_PER_MEMBER_PER_N_BLOCK.amount;
        limit_per_memeber /= SCORUM_REGISTRATION_BONUS_LIMIT_PER_MEMBER_N_BLOCK;
        FC_ASSERT(member.already_allocated_cash + per_reg <= asset(limit_per_memeber, SCORUM_SYMBOL),
                  "Committee member '${1}' reaches cash limit.", ("1", member_name));
    }

    // return value <= per_reg
    per_reg = decrease_balance(per_reg);

    auto modifier = [=](registration_committee_member_object& m) {
        m.last_allocated_block = head_block_num;
        if (per_n_block_remain > 0)
        {
            m.per_n_block_remain = per_n_block_remain;
            m.already_allocated_cash += per_reg;
        }
        else
        {
            m.per_n_block_remain = SCORUM_REGISTRATION_BONUS_LIMIT_PER_MEMBER_N_BLOCK;
            m.already_allocated_cash = asset(0, SCORUM_SYMBOL);
        }
    };
    committee_service.update_member_info(member, modifier);

    if (!check_autoclose())
    {
        increase_already_allocated_count();
    }

    return per_reg;
}

void dbs_registration_pool::_close()
{
    const registration_pool_object& this_pool = get();
    db_impl().remove(this_pool);
}
} // namespace chain
} // namespace scorum
