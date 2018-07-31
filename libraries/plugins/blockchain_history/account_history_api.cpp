#include <scorum/blockchain_history/account_history_api.hpp>
#include <scorum/blockchain_history/blockchain_history_plugin.hpp>
#include <scorum/blockchain_history/schema/account_history_object.hpp>
#include <scorum/app/api_context.hpp>
#include <scorum/app/application.hpp>
#include <scorum/blockchain_history/schema/operation_objects.hpp>
#include <scorum/common_api/config.hpp>
#include <scorum/protocol/operations.hpp>

#include <map>

namespace scorum {
namespace blockchain_history {

namespace detail {
class account_history_api_impl
{
public:
    scorum::app::application& _app;

public:
    account_history_api_impl(scorum::app::application& app)
        : _app(app)
    {
    }

    template <typename history_object_type, typename fill_result_functor>
    void get_history(const std::string& account, uint64_t from, uint32_t limit, fill_result_functor& funct) const
    {
        const auto db = _app.chain_database();

        FC_ASSERT(limit > 0, "Limit must be greater than zero");
        FC_ASSERT(limit <= MAX_BLOCKCHAIN_HISTORY_DEPTH, "Limit of ${l} is greater than maxmimum allowed ${2}",
                  ("l", limit)("2", MAX_BLOCKCHAIN_HISTORY_DEPTH));
        FC_ASSERT(from >= limit, "From must be greater than limit");

        const auto& idx = db->get_index<history_index<history_object_type>>().indices().get<by_account>();
        auto itr = idx.lower_bound(boost::make_tuple(account, from));
        if (itr != idx.end())
        {
            auto end = idx.upper_bound(boost::make_tuple(account, int64_t(0)));
            int64_t pos = int64_t(itr->sequence) - limit;
            if (pos > 0)
            {
                end = idx.lower_bound(boost::make_tuple(account, pos));
            }
            while (itr != end)
            {
                funct(*itr);
                ++itr;
            }
        }
    }

    template <typename history_object_type>
    std::map<uint32_t, applied_operation> get_history(const std::string& account, uint64_t from, uint32_t limit) const
    {
        std::map<uint32_t, applied_operation> result;

        const auto db = _app.chain_database();

        auto fill_funct = [&](const history_object_type& hobj) { result[hobj.sequence] = db->get(hobj.op); };
        this->template get_history<history_object_type>(account, from, limit, fill_funct);

        return result;
    }
};
} // namespace detail

account_history_api::account_history_api(const scorum::app::api_context& ctx)
    : _impl(new detail::account_history_api_impl(ctx.app))
{
}

account_history_api::~account_history_api()
{
}

void account_history_api::on_api_startup()
{
}

std::map<uint32_t, applied_operation>
account_history_api::get_account_scr_to_scr_transfers(const std::string& account, uint64_t from, uint32_t limit) const
{
    const auto db = _impl->_app.chain_database();
    return db->with_read_lock(
        [&]() { return _impl->get_history<transfers_to_scr_history_object>(account, from, limit); });
}

std::map<uint32_t, applied_operation>
account_history_api::get_account_scr_to_sp_transfers(const std::string& account, uint64_t from, uint32_t limit) const
{
    const auto db = _impl->_app.chain_database();
    return db->with_read_lock(
        [&]() { return _impl->get_history<transfers_to_sp_history_object>(account, from, limit); });
}

std::map<uint32_t, applied_operation>
account_history_api::get_account_history(const std::string& account, uint64_t from, uint32_t limit) const
{
    const auto db = _impl->_app.chain_database();
    return db->with_read_lock([&]() { return _impl->get_history<account_history_object>(account, from, limit); });
}

std::map<uint32_t, applied_withdraw_operation>
account_history_api::get_account_sp_to_scr_transfers(const std::string& account, uint64_t from, uint32_t limit) const
{
    const auto db = _impl->_app.chain_database();
    return db->with_read_lock([&]() {
        std::map<uint32_t, applied_withdraw_operation> result;

        auto fill_funct = [&](const withdrawals_to_scr_history_object& obj) {
            auto it = result.emplace(obj.sequence, applied_withdraw_operation(db->get(obj.op))).first;
            auto& applied_op = it->second;

            share_type to_withdraw = 0;
            applied_op.op.weak_visit(
                [&](const withdraw_scorumpower_operation& op) { to_withdraw = op.scorumpower.amount; });

            if (to_withdraw == 0u)
            {
                // If this is a zero-withdraw (such withdraw closes current active withdraw)
                applied_op.status = applied_withdraw_operation::empty;
            }
            else if (!obj.progress.empty())
            {
                auto last_op = fc::raw::unpack<operation>(db->get(obj.progress.back()).serialized_op);

                last_op.weak_visit(
                    [&](const acc_finished_vesting_withdraw_operation&) {
                        // if last 'progress' operation is 'acc_finished_' then withdraw was finished
                        applied_op.status = applied_withdraw_operation::finished;
                    },
                    [&](const withdraw_scorumpower_operation&) {
                        // if last 'progress' operation is 'withdraw_scorumpower_' then withdraw was either interrupted
                        // or finished depending on pre-last 'progress' operation
                        applied_op.status = applied_withdraw_operation::interrupted;
                    });

                if (obj.progress.size() > 1)
                {
                    auto before_last_op_obj = db->get(*(obj.progress.rbegin() + 1));
                    auto before_last_op = fc::raw::unpack<operation>(before_last_op_obj.serialized_op);

                    before_last_op.weak_visit([&](const acc_finished_vesting_withdraw_operation&) {
                        // if pre-last 'progress' operation is 'acc_finished_' then withdraw was finished
                        applied_op.status = applied_withdraw_operation::finished;
                    });
                }

                for (auto& id : obj.progress)
                {
                    auto op = fc::raw::unpack<operation>(db->get(id).serialized_op);

                    op.weak_visit(
                        [&](const acc_to_acc_vesting_withdraw_operation& op) {
                            applied_op.withdrawn += op.withdrawn.amount;
                        },
                        [&](const acc_to_devpool_vesting_withdraw_operation& op) {
                            applied_op.withdrawn += op.withdrawn.amount;
                        });
                }
            }
        };
        _impl->get_history<withdrawals_to_scr_history_object>(account, from, limit, fill_funct);

        return result;
    });
}

} // namespace blockchain_history
} // namespace scorum
