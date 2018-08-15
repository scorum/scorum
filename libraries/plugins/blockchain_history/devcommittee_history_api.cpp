#include <scorum/blockchain_history/devcommittee_history_api.hpp>
#include <scorum/blockchain_history/blockchain_history_plugin.hpp>
#include <scorum/blockchain_history/schema/history_object.hpp>
#include <scorum/app/api_context.hpp>
#include <scorum/app/application.hpp>
#include <scorum/blockchain_history/schema/operation_objects.hpp>
#include <scorum/common_api/config.hpp>
#include <scorum/protocol/operations.hpp>

#include <map>

namespace scorum {
namespace blockchain_history {

namespace detail {
class devcommittee_history_api_impl
{
public:
    scorum::app::application& _app;

public:
    devcommittee_history_api_impl(scorum::app::application& app)
        : _app(app)
    {
    }

    template <typename history_object_type, typename fill_result_functor>
    void get_history(uint64_t from, uint32_t limit, fill_result_functor& funct) const
    {
        const auto db = _app.chain_database();

        FC_ASSERT(limit > 0, "Limit must be greater than zero");
        FC_ASSERT(limit <= MAX_BLOCKCHAIN_HISTORY_DEPTH, "Limit of ${l} is greater than maxmimum allowed ${2} ",
                  ("l", limit)("2", MAX_BLOCKCHAIN_HISTORY_DEPTH));
        FC_ASSERT(from >= limit, "From must be greater than limit");

        const auto& idx = db->get_index<devcommittee_history_index<history_object_type>, by_id_desc>();

        for (auto it = idx.lower_bound(from); it != idx.end() && limit; ++it, --limit)
        {
            funct(*it);
        }
    }

    template <typename history_object_type>
    std::vector<applied_operation> get_history(uint64_t from, uint32_t limit) const
    {
        std::vector<applied_operation> result;

        const auto db = _app.chain_database();

        auto fill_funct = [&](const history_object_type& hobj) { result.emplace_back(db->get(hobj.op)); };
        this->template get_history<history_object_type>(from, limit, fill_funct);

        return result;
    }
};
} // namespace detail

devcommittee_history_api::devcommittee_history_api(const scorum::app::api_context& ctx)
    : _impl(new detail::devcommittee_history_api_impl(ctx.app))
{
}

devcommittee_history_api::~devcommittee_history_api() = default;

void devcommittee_history_api::on_api_startup()
{
}

std::vector<applied_operation> devcommittee_history_api::get_scr_to_scr_transfers(uint64_t from, uint32_t limit) const
{
    const auto db = _impl->_app.chain_database();
    return db->with_read_lock(
        [&]() { return _impl->get_history<devcommittee_transfers_to_scr_history_object>(from, limit); });
}

std::vector<applied_operation> devcommittee_history_api::get_history(uint64_t from, uint32_t limit) const
{
    const auto db = _impl->_app.chain_database();
    return db->with_read_lock([&]() { return _impl->get_history<devcommittee_history_object>(from, limit); });
}

std::vector<applied_withdraw_operation> devcommittee_history_api::get_sp_to_scr_transfers(uint64_t from,
                                                                                          uint32_t limit) const
{
    const auto db = _impl->_app.chain_database();
    return db->with_read_lock([&]() {
        std::vector<applied_withdraw_operation> result;

        auto fill_funct = [&](const devcommittee_withdrawals_to_scr_history_object& obj) {
            result.emplace_back(db->get(obj.op));
            auto& applied_op = result.back();

            share_type to_withdraw = 0;
            applied_op.op.weak_visit([&](const proposal_virtual_operation& proposal_op) {
                proposal_op.proposal_op.weak_visit([&](const development_committee_withdraw_vesting_operation& op) {
                    to_withdraw = op.vesting_shares.amount;
                });
            });

            if (to_withdraw == 0u)
            {
                // If this is a zero-withdraw (such withdraw closes current active withdraw)
                applied_op.status = applied_withdraw_operation::empty;
            }
            else if (!obj.progress.empty())
            {
                auto last_op = fc::raw::unpack<operation>(db->get(obj.progress.back()).serialized_op);

                last_op.weak_visit(
                    [&](const devpool_finished_vesting_withdraw_operation&) {
                        // if last 'progress' operation is 'devpool_finished_' then withdraw was finished
                        applied_op.status = applied_withdraw_operation::finished;
                    },
                    [&](const proposal_virtual_operation& proposal_op) {
                        FC_ASSERT(
                            proposal_op.proposal_op.which()
                                == proposal_operation::tag<development_committee_withdraw_vesting_operation>::value,
                            "The only proposal_virtual_operation which can be in withdraw progress is a "
                            "'development_committee_withdraw_vesting_operation' one");
                        // if last 'progress' operation is 'proposal_virtual_operation' then withdraw was either
                        // interrupted or finished depending on pre-last 'progress' operation
                        applied_op.status = applied_withdraw_operation::interrupted;
                    });

                if (obj.progress.size() > 1)
                {
                    auto before_last_op_obj = db->get(*(obj.progress.rbegin() + 1));
                    auto before_last_op = fc::raw::unpack<operation>(before_last_op_obj.serialized_op);

                    before_last_op.weak_visit([&](const devpool_finished_vesting_withdraw_operation&) {
                        // if pre-last 'progress' operation is 'acc_finished_' then withdraw was finished
                        applied_op.status = applied_withdraw_operation::finished;
                    });
                }

                for (auto& id : obj.progress)
                {
                    auto op = fc::raw::unpack<operation>(db->get(id).serialized_op);

                    op.weak_visit([&](const devpool_to_devpool_vesting_withdraw_operation& op) {
                        applied_op.withdrawn += op.withdrawn.amount;
                    });
                }
            }
        };
        _impl->get_history<devcommittee_withdrawals_to_scr_history_object>(from, limit, fill_funct);

        return result;
    });
}

} // namespace blockchain_history
} // namespace scorum
