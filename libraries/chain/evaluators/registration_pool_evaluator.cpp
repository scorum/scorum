#include <scorum/chain/evaluators/registration_pool_evaluator.hpp>

#include <scorum/chain/data_service_factory.hpp>

#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/registration_pool.hpp>
#include <scorum/chain/services/registration_committee.hpp>
#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/chain/services/account_registration_bonus.hpp>

#include <scorum/chain/schema/account_objects.hpp>
#include <scorum/chain/schema/registration_objects.hpp>
#include <scorum/chain/schema/dynamic_global_property_object.hpp>

#include <fc/exception/exception.hpp>
#include <fc/log/logger.hpp>

namespace scorum {
namespace chain {

using scorum::protocol::asset;

class registration_pool_evaluator_impl
{
public:
    registration_pool_evaluator_impl(data_service_factory_i& services)
        : _account_service(services.account_service())
        , _registration_pool_service(services.registration_pool_service())
        , _registration_committee_service(services.registration_committee_service())
        , _dprops_service(services.dynamic_global_property_service())
        , _account_registration_bonus_service(services.account_registration_bonus_service())
    {
    }

    asset allocate_cash(const account_name_type& committee_member = account_name_type())
    {
        const registration_pool_object& pool = _registration_pool_service.get();

        asset per_reg = calculate_amount_by_schedule(pool);
        FC_ASSERT(per_reg.amount > 0, "Invalid schedule. Zero bonus return.");

        // return value <= per_reg
        per_reg = take_actually_amount(pool, per_reg);

        FC_ASSERT(per_reg.amount > 0, "Nothing to allocate.");

        if (committee_member != account_name_type())
        {
            validate_bandwidth(pool, committee_member, _registration_committee_service.get_member(committee_member),
                               per_reg);
        }

        _registration_pool_service.decrease_balance(per_reg);

        _registration_pool_service.increase_already_allocated_count();

        return per_reg;
    }

    void register_account_bonus(const account_name_type& new_account, const asset& bonus)
    {
        _account_service.check_account_existence(new_account);

        FC_ASSERT(bonus.amount > 0, "No bonus to register.");

        _account_registration_bonus_service.create([&](account_registration_bonus_object& a) {
            a.account = new_account;
            a.bonus += asset(bonus.amount, SP_SYMBOL);
            a.expires = _dprops_service.head_block_time() + SCORUM_EXPIRATON_FOR_REGISTRATION_BONUS.to_seconds();
        });
    }

private:
    asset calculate_amount_by_schedule(const registration_pool_object& pool)
    {
        FC_ASSERT(!pool.schedule_items.empty(), "Invalid schedule.");

        // find position in schedule
        std::size_t ci = 0;
        uint64_t allocated_rest = pool.already_allocated_count;
        auto it = pool.schedule_items.begin();
        for (; it != pool.schedule_items.end(); ++it, ++ci)
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

        if (it == pool.schedule_items.end())
        {
            // no more schedule items (out of schedule),
            // use last stage to calculate bonus
            --it;
        }

        using schedule_item_type = registration_pool_object::schedule_item;
        const schedule_item_type& current_item = (*it);
        return asset(current_item.bonus_percent * pool.maximum_bonus.amount / 100, SCORUM_SYMBOL);
    }

    asset take_actually_amount(const registration_pool_object& pool, const asset& balance)
    {
        FC_ASSERT(balance.amount > 0, "Invalid balance.");

        asset ret(0, SCORUM_SYMBOL);

        if (pool.balance.amount > 0 && balance <= pool.balance)
        {
            ret = balance;
        }
        else
        {
            ret = pool.balance;
        }

        return ret;
    }

    void check_bandwidth(const registration_pool_object& pool,
                         const account_name_type& member_name,
                         uint32_t pass_blocks,
                         const asset& allocated_cash)
    {
        share_type limit_per_member = (share_type)(pass_blocks + 1);
        limit_per_member *= pool.maximum_bonus.amount;
        limit_per_member *= SCORUM_REGISTRATION_BONUS_LIMIT_PER_MEMBER_PER_N_BLOCK;
        limit_per_member /= SCORUM_REGISTRATION_BONUS_LIMIT_PER_MEMBER_N_BLOCK;
        FC_ASSERT(allocated_cash <= asset(limit_per_member, SCORUM_SYMBOL),
                  "Committee member '${1}' reaches cash limit.", ("1", member_name));
    }

    void validate_bandwidth(const registration_pool_object& pool,
                            const account_name_type& member_name,
                            const registration_committee_member_object& member,
                            const asset& amount)
    {
        const dynamic_global_property_object& dprops = _dprops_service.get();
        uint32_t head_block_num = dprops.head_block_number;

        uint32_t last_allocated_block = member.last_allocated_block;
        FC_ASSERT(last_allocated_block <= head_block_num);

        uint32_t pass_blocks = 0;
        if (last_allocated_block > 0)
        {
            pass_blocks = head_block_num - last_allocated_block;
        }

        last_allocated_block = head_block_num;

        // move floating window position
        uint32_t per_n_block_remain = member.per_n_block_remain;
        if (pass_blocks > per_n_block_remain)
        {
            per_n_block_remain = 0;
        }
        else if (pass_blocks > 0)
        {
            per_n_block_remain -= pass_blocks;
        }

        asset already_allocated_cash(0, SCORUM_SYMBOL);
        asset allocated_cash = member.already_allocated_cash;
        if (per_n_block_remain > 0)
        {
            allocated_cash += amount;
            already_allocated_cash = allocated_cash;
        }
        else
        {
            allocated_cash = asset(0, SCORUM_SYMBOL); // to prevent checking at the begining of floating window
            already_allocated_cash = amount;
            per_n_block_remain = SCORUM_REGISTRATION_BONUS_LIMIT_PER_MEMBER_N_BLOCK;
        }

        check_bandwidth(pool, member_name, pass_blocks, allocated_cash);

        auto modifier = [=](registration_committee_member_object& m) {
            m.last_allocated_block = last_allocated_block;
            m.per_n_block_remain = per_n_block_remain;
            m.already_allocated_cash = already_allocated_cash;
        };

        _registration_committee_service.update_member_info(member, modifier);
    }

    account_service_i& _account_service;
    registration_pool_service_i& _registration_pool_service;
    registration_committee_service_i& _registration_committee_service;
    dynamic_global_property_service_i& _dprops_service;
    account_registration_bonus_service_i& _account_registration_bonus_service;
};

//

registration_pool_evaluator::registration_pool_evaluator(data_service_factory_i& services)
    : evaluator_impl<data_service_factory_i, registration_pool_evaluator>(services)
    , _impl(new registration_pool_evaluator_impl(services))
    , _account_service(services.account_service())
    , _registration_pool_service(services.registration_pool_service())
    , _registration_committee_service(services.registration_committee_service())
    , _dprops_service(services.dynamic_global_property_service())
{
}

registration_pool_evaluator::~registration_pool_evaluator()
{
}

void registration_pool_evaluator::do_apply(const registration_pool_evaluator::operation_type& o)
{
    _account_service.check_account_existence(o.creator);

    _account_service.check_account_existence(o.owner.account_auths);

    _account_service.check_account_existence(o.active.account_auths);

    _account_service.check_account_existence(o.posting.account_auths);

    FC_ASSERT(_registration_pool_service.is_exists(), "Registration pool is not initialized.");

    FC_ASSERT(_registration_pool_service.get().balance.amount > 0, "Registration pool is exhausted.");

    FC_ASSERT(_registration_committee_service.is_exists(o.creator), "Account '${1}' is not committee member.",
              ("1", o.creator));

    asset bonus = _impl->allocate_cash(o.creator);

    _account_service.create_account_with_bonus(o.new_account_name, o.creator, o.memo_key, o.json_metadata, o.owner,
                                               o.active, o.posting, bonus);
    _impl->register_account_bonus(o.new_account_name, bonus);
}

//

give_bonus_from_registration_pool_task_context::give_bonus_from_registration_pool_task_context(
    data_service_factory_i& services, const account_object& beneficiary)
    : _services(services)
    , _beneficiary(beneficiary)
{
}

data_service_factory_i& give_bonus_from_registration_pool_task_context::services() const
{
    return _services;
}

const account_object& give_bonus_from_registration_pool_task_context::beneficiary() const
{
    return _beneficiary;
}

void give_bonus_from_registration_pool_task::on_apply(give_bonus_from_registration_pool_task_context& ctx)
{
    registration_pool_evaluator_impl impl(ctx.services());

    asset bonus = impl.allocate_cash();
    account_service_i& account_service = ctx.services().account_service();

    account_service.create_scorumpower(ctx.beneficiary(), bonus);
    impl.register_account_bonus(ctx.beneficiary().name, bonus);
}
}
}
