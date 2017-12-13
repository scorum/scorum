#include <scorum/chain/dbs_registration_pool.hpp>
#include <scorum/chain/database.hpp>

#include <scorum/chain/dbs_registration_committee.hpp>
#include <scorum/chain/dbs_account.hpp>

namespace scorum {
namespace chain {

dbs_registration_pool::dbs_registration_pool(database& db)
    : _base_type(db)
{
}

bool dbs_registration_pool::is_pool_exists() const
{
    return !db_impl().get_index<registration_pool_index>().indicies().empty();
}

const registration_pool_object& dbs_registration_pool::get_pool() const
{
    const auto& idx = db_impl().get_index<registration_pool_index>().indicies();
    auto it = idx.cbegin();
    FC_ASSERT(it != idx.cend(), "Pool is not found.");
    return (*it);
}

const registration_pool_object& dbs_registration_pool::create_pool(const genesis_state_type& genesis_state)
{
    FC_ASSERT(genesis_state.registration_supply > 0, "Registration supply amount must be more than zerro.");
    FC_ASSERT(genesis_state.registration_maximum_bonus > 0,
              "Registration maximum bonus amount must be more than zerro.");
    FC_ASSERT(!genesis_state.registration_schedule.empty(), "Registration schedule must have at least one item.");

    // create sorted items list form genesis unordered data
    using schedule_item_type = registration_pool_object::schedule_item;
    using sorted_type = std::map<uint8_t, schedule_item_type>;
    sorted_type items;
    for (const auto& genesis_item : genesis_state.registration_schedule)
    {
        FC_ASSERT(genesis_item.users > 0, "Invalid schedule value (users in thousands) for stage ${1}.",
                  ("1", genesis_item.stage));
        FC_ASSERT(genesis_item.bonus_percent >= 0 && genesis_item.bonus_percent <= 100,
                  "Invalid schedule value (percent) for stage ${1}.", ("1", genesis_item.stage));
        items.insert(sorted_type::value_type(genesis_item.stage,
                                             schedule_item_type{ genesis_item.users, genesis_item.bonus_percent }));
    }

    // check existence here to allow unit tests check input data even if object exists in DB
    FC_ASSERT(!is_pool_exists(), "Can't create more than one pool.");

    // create pool
    const auto& new_pool = db_impl().create<registration_pool_object>([&](registration_pool_object& pool) {
        pool.balance = asset(genesis_state.registration_supply, VESTS_SYMBOL);
        pool.maximum_bonus = genesis_state.registration_maximum_bonus;
        pool.schedule_items.reserve(items.size());
        for (const auto& item : items)
        {
            pool.schedule_items.push_back(item.second);
        }
    });

    return new_pool;
}

asset dbs_registration_pool::allocate_cash(const account_name_type& member_name)
{
    if (!is_pool_exists())
    {
        return asset(0, VESTS_SYMBOL);
    }

    FC_ASSERT(SCORUM_REGISTRATION_BONUS_LIMIT_PER_MEMBER_N_BLOCK > 0, "Invalid ${1} value.",
              ("1", SCORUM_REGISTRATION_BONUS_LIMIT_PER_MEMBER_N_BLOCK));
    FC_ASSERT(SCORUM_REGISTRATION_BONUS_LIMIT_PER_MEMBER_PER_N_BLOCK_AMOUNT > 0, "Invalid ${1} value.",
              ("1", SCORUM_REGISTRATION_BONUS_LIMIT_PER_MEMBER_PER_N_BLOCK_AMOUNT));

    const dbs_account& account_service = db().obtain_service<dbs_account>();

    account_service.check_account_existence(member_name);

    const registration_pool_object& this_pool = get_pool();

    share_type per_reg = _calculate_per_reg(this_pool);
    FC_ASSERT(per_reg > 0, "Invalid schedule. Zero bonus return.");
    FC_ASSERT(per_reg <= SCORUM_REGISTRATION_BONUS_LIMIT_PER_MEMBER_PER_N_BLOCK_AMOUNT
                      / SCORUM_REGISTRATION_BONUS_LIMIT_PER_MEMBER_N_BLOCK,
              "Invalid schedule. Always exceeds the limit.");

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

    uint32_t per_n_block_rest = member.per_n_block_rest;
    if (pass_blocks > per_n_block_rest)
    {
        per_n_block_rest = 0;
    }
    else
    {
        per_n_block_rest -= pass_blocks;
    }

    if (per_n_block_rest > 0)
    {
        // check limits
        share_type limit_per_memeber = (share_type)(pass_blocks + 1);
        limit_per_memeber *= SCORUM_REGISTRATION_BONUS_LIMIT_PER_MEMBER_PER_N_BLOCK_AMOUNT;
        limit_per_memeber /= SCORUM_REGISTRATION_BONUS_LIMIT_PER_MEMBER_N_BLOCK;
        FC_ASSERT(member.already_allocated_cash + per_reg <= limit_per_memeber,
                  "Committee member '${1}' reaches cash limit.", ("1", member_name));
    }

    per_reg = _decrease_balance(this_pool, per_reg);

    auto modifier = [=](registration_committee_member_object& m) {
        m.last_allocated_block = head_block_num;
        if (per_n_block_rest > 0)
        {
            m.per_n_block_rest = per_n_block_rest;
            m.already_allocated_cash += per_reg;
        }
        else
        {
            m.per_n_block_rest = SCORUM_REGISTRATION_BONUS_LIMIT_PER_MEMBER_N_BLOCK;
            m.already_allocated_cash = 0;
        }
    };
    committee_service.update_member_info(member, modifier);

    if (!_check_autoclose(this_pool))
    {
        db_impl().modify(this_pool, [&](registration_pool_object& pool) { pool.already_allocated_count++; });
    }

    return asset(per_reg, VESTS_SYMBOL);
}

share_type dbs_registration_pool::_decrease_balance(const registration_pool_object& this_pool,
                                                    const share_type& balance)
{
    FC_ASSERT(balance > 0, "Invalid balance.");

    share_type ret{ 0 };

    db_impl().modify(this_pool, [&](registration_pool_object& pool) {
        if (pool.balance.amount > 0 && balance <= pool.balance.amount)
        {
            pool.balance.amount -= balance;
            ret = balance;
        }
        else
        {
            ret = pool.balance.amount;
            pool.balance.amount = 0;
        }
    });

    return ret;
}

share_type dbs_registration_pool::_calculate_per_reg(const registration_pool_object& this_pool)
{
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
    return current_item.bonus_percent * this_pool.maximum_bonus / 100;
}

bool dbs_registration_pool::_check_autoclose(const registration_pool_object& this_pool)
{
    if (this_pool.balance.amount <= 0)
    {
        _close(this_pool);
        return true;
    }
    else
    {
        return false;
    }
}

void dbs_registration_pool::_close(const registration_pool_object& this_pool)
{
    db_impl().remove(this_pool);
}
}
}
