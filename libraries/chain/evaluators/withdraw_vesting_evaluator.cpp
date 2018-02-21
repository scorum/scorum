#include <scorum/chain/evaluators/withdraw_vesting_evaluator.hpp>

#include <scorum/chain/data_service_factory.hpp>

#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/chain/services/withdraw_vesting.hpp>

#include <scorum/chain/schema/account_objects.hpp>
#include <scorum/chain/schema/dynamic_global_property_object.hpp>
#include <scorum/chain/schema/withdraw_vesting_objects.hpp>

namespace scorum {
namespace chain {

withdraw_vesting_evaluator::withdraw_vesting_evaluator(data_service_factory_i& services)
    : evaluator_impl<data_service_factory_i, withdraw_vesting_evaluator>(services)
    , _account_service(db().account_service())
    , _dprops_service(db().dynamic_global_property_service())
    , _withdraw_vesting_service(db().withdraw_vesting_service())
{
}

void withdraw_vesting_evaluator::do_apply(const withdraw_vesting_evaluator::operation_type& o)
{
    const auto& account = _account_service.get_account(o.account);

    FC_ASSERT(account.vesting_shares >= asset(0, VESTS_SYMBOL),
              "Account does not have sufficient Scorum Power for withdraw.");
    FC_ASSERT(account.vesting_shares - account.delegated_vesting_shares >= o.vesting_shares,
              "Account does not have sufficient Scorum Power for withdraw.");

    if (!account.created_by_genesis)
    {
        const auto& dprops = _dprops_service.get();
        asset min_vests = asset(dprops.median_chain_props.account_creation_fee.amount, VESTS_SYMBOL);
        min_vests *= SCORUM_START_WITHDRAW_COEFFICIENT;

        FC_ASSERT(account.vesting_shares > min_vests || o.vesting_shares.amount == 0,
                  "Account registered by another account requires 10x account creation fee worth of Scorum Power "
                  "before it can be powered down.");
    }

    asset vesting_withdraw_rate = asset(0, VESTS_SYMBOL);
    if (_withdraw_vesting_service.is_exists(account.id))
    {
        vesting_withdraw_rate = _withdraw_vesting_service.get(account.id).vesting_withdraw_rate;
    }

    if (o.vesting_shares.amount == 0)
    {
        FC_ASSERT(vesting_withdraw_rate.amount != 0, "This operation would not change the vesting withdraw rate.");

        remove_withdraw_vesting(account);
    }
    else
    {
        // SCORUM: We have to decide whether we use 13 weeks vesting period or low it down
        // 13 weeks = 1 quarter of a year
        auto new_vesting_withdraw_rate = o.vesting_shares / SCORUM_VESTING_WITHDRAW_INTERVALS;

        if (new_vesting_withdraw_rate.amount == 0)
            new_vesting_withdraw_rate.amount = 1;

        FC_ASSERT(vesting_withdraw_rate != new_vesting_withdraw_rate,
                  "This operation would not change the vesting withdraw rate.");

        auto lmbNewVesting = [&](withdraw_vesting_object& wv) {
            wv.vesting_withdraw_rate = new_vesting_withdraw_rate;
            wv.next_vesting_withdrawal
                = _dprops_service.head_block_time() + fc::seconds(SCORUM_VESTING_WITHDRAW_INTERVAL_SECONDS);
            wv.to_withdraw = o.vesting_shares;
            wv.withdrawn = asset(0, VESTS_SYMBOL);
        };

        if (!_withdraw_vesting_service.is_exists(account.id))
        {
            _withdraw_vesting_service.create(lmbNewVesting);
        }
        else
        {
            const withdraw_vesting_object& wv = _withdraw_vesting_service.get(account.id);
            _withdraw_vesting_service.update(wv, lmbNewVesting);
        }
    }
}

void withdraw_vesting_evaluator::remove_withdraw_vesting(const account_object& from_account)
{
    if (_withdraw_vesting_service.is_exists(from_account.id))
    {
        const withdraw_vesting_object& wv = _withdraw_vesting_service.get(from_account.id);
        _withdraw_vesting_service.remove(wv);
    }
}
}
}
