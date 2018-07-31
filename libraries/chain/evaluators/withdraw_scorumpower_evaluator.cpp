#include <scorum/chain/evaluators/withdraw_scorumpower_evaluator.hpp>

#include <scorum/chain/data_service_factory.hpp>

#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/chain/services/withdraw_scorumpower.hpp>
#include <scorum/chain/services/dev_pool.hpp>
#include <scorum/chain/services/genesis_state.hpp>

#include <scorum/chain/schema/account_objects.hpp>
#include <scorum/chain/schema/dynamic_global_property_object.hpp>
#include <scorum/chain/schema/withdraw_scorumpower_objects.hpp>
#include <scorum/chain/schema/dev_committee_object.hpp>

namespace scorum {
namespace chain {

class withdraw_scorumpower_impl
{
public:
    withdraw_scorumpower_impl(data_service_factory_i& services)
        : _dprops_service(services.dynamic_global_property_service())
        , _withdraw_scorumpower_service(services.withdraw_scorumpower_service())
        , _lock_withdraw_sp_until_timestamp(services.genesis_state_service().get_lock_withdraw_sp_until_timestamp())
    {
    }

    template <typename FromObjectType> void do_apply(const FromObjectType& from_object, const asset& scorumpower)
    {
        FC_ASSERT(_dprops_service.head_block_time() > _lock_withdraw_sp_until_timestamp,
                  "Withdraw scorumpower operation is locked until ${t}.", ("t", _lock_withdraw_sp_until_timestamp));
        FC_ASSERT(scorumpower.amount >= 0);

        asset vesting_withdraw_rate = asset(0, SP_SYMBOL);
        if (_withdraw_scorumpower_service.is_exists(from_object.id))
        {
            vesting_withdraw_rate = _withdraw_scorumpower_service.get(from_object.id).vesting_withdraw_rate;
        }

        if (scorumpower.amount == 0)
        {
            FC_ASSERT(vesting_withdraw_rate.amount != 0, "This operation would not change the vesting withdraw rate.");

            remove_withdraw_scorumpower(from_object);
        }
        else
        {
            // SCORUM: We have to decide whether we use 13 weeks vesting period or low it down
            // 13 weeks = 1 quarter of a year
            auto new_vesting_withdraw_rate = scorumpower / SCORUM_VESTING_WITHDRAW_INTERVALS;

            if (new_vesting_withdraw_rate.amount == 0)
                new_vesting_withdraw_rate.amount = 1;

            auto lmbNewVesting = [&](withdraw_scorumpower_object& wv) {
                wv.from_id = from_object.id;
                wv.vesting_withdraw_rate = new_vesting_withdraw_rate;
                wv.next_vesting_withdrawal
                    = _dprops_service.head_block_time() + fc::seconds(SCORUM_VESTING_WITHDRAW_INTERVAL_SECONDS);
                wv.to_withdraw = scorumpower;
                wv.withdrawn = asset(0, SP_SYMBOL);
            };

            if (!_withdraw_scorumpower_service.is_exists(from_object.id))
            {
                _withdraw_scorumpower_service.create(lmbNewVesting);
            }
            else
            {
                const withdraw_scorumpower_object& wv = _withdraw_scorumpower_service.get(from_object.id);
                _withdraw_scorumpower_service.update(wv, lmbNewVesting);
            }
        }
    }

    template <typename FromObjectType> void remove_withdraw_scorumpower(const FromObjectType& from_object)
    {
        if (_withdraw_scorumpower_service.is_exists(from_object.id))
        {
            const withdraw_scorumpower_object& wv = _withdraw_scorumpower_service.get(from_object.id);
            _withdraw_scorumpower_service.remove(wv);
        }
    }

private:
    dynamic_global_property_service_i& _dprops_service;
    withdraw_scorumpower_service_i& _withdraw_scorumpower_service;
    const fc::time_point_sec _lock_withdraw_sp_until_timestamp;
};

//

withdraw_scorumpower_evaluator::withdraw_scorumpower_evaluator(data_service_factory_i& services)
    : evaluator_impl<data_service_factory_i, withdraw_scorumpower_evaluator>(services)
    , _impl(new withdraw_scorumpower_impl(services))
    , _account_service(db().account_service())
    , _dprops_service(db().dynamic_global_property_service())
{
}

withdraw_scorumpower_evaluator::~withdraw_scorumpower_evaluator()
{
}

void withdraw_scorumpower_evaluator::do_apply(const withdraw_scorumpower_evaluator::operation_type& o)
{
    const auto& account = _account_service.get_account(o.account);

    FC_ASSERT(account.scorumpower >= asset(0, SP_SYMBOL),
              "Account does not have sufficient Scorum Power for withdraw.");
    FC_ASSERT(account.scorumpower - account.delegated_scorumpower >= o.scorumpower,
              "Account does not have sufficient Scorum Power for withdraw.");

    if (!account.created_by_genesis)
    {
        const auto& dprops = _dprops_service.get();
        asset min_scorumpower = asset(dprops.median_chain_props.account_creation_fee.amount, SP_SYMBOL);
        min_scorumpower *= SCORUM_START_WITHDRAW_COEFFICIENT;

        FC_ASSERT(account.scorumpower > min_scorumpower || o.scorumpower.amount == 0,
                  "Account registered by another account requires 10x account creation fee worth of Scorum Power "
                  "before it can be powered down.");
    }

    _impl->do_apply(account, o.scorumpower);
}

//

withdraw_scorumpower_context::withdraw_scorumpower_context(data_service_factory_i& services, const asset& scorumpower)
    : _services(services)
    , _scorumpower(scorumpower)
{
}

void withdraw_scorumpower_dev_pool_task::on_apply(withdraw_scorumpower_context& ctx)
{
    withdraw_scorumpower_impl impl(ctx.services());

    dev_pool_service_i& dev_pool_service = ctx.services().dev_pool_service();

    const auto& pool = dev_pool_service.get();

    FC_ASSERT(pool.sp_balance >= ctx.scorumpower(), "Dev pool does not have sufficient Scorum Power for withdraw.");

    impl.do_apply(pool, ctx.scorumpower());
}
}
}
