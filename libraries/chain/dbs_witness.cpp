#include <scorum/chain/dbs_witness.hpp>
#include <scorum/chain/database.hpp>

#include <scorum/chain/witness_objects.hpp>
#include <scorum/chain/account_object.hpp>

namespace scorum {
namespace chain {

dbs_witness::dbs_witness(database& db)
    : _base_type(db)
{
}

const witness_object& dbs_witness::get_witness(const account_name_type& name) const
{
    try
    {
        return db_impl().get<witness_object, by_name>(name);
    }
    FC_CAPTURE_AND_RETHROW((name))
}

const witness_schedule_object& dbs_witness::get_witness_schedule_object() const
{
    try
    {
        return db_impl().get<witness_schedule_object>();
    }
    FC_CAPTURE_AND_RETHROW()
}

const witness_object& dbs_witness::get_top_witness() const
{
    const auto& idx = db_impl().get_index<witness_index>().indices().get<by_vote_name>();
    FC_ASSERT(idx.begin() != idx.end(), "Empty witness_index by_vote_name.");
    return (*idx.begin());
}

void dbs_witness::adjust_witness_votes(const account_object& account, share_type delta)
{
    const auto& vidx = db_impl().get_index<witness_vote_index>().indices().get<by_account_witness>();
    auto itr = vidx.lower_bound(boost::make_tuple(account.id, witness_id_type()));
    while (itr != vidx.end() && itr->account == account.id)
    {
        adjust_witness_vote(db_impl().get(itr->witness), delta);
        ++itr;
    }
}

void dbs_witness::adjust_witness_vote(const witness_object& witness, share_type delta)
{
    const auto& props = db_impl().get_dynamic_global_properties();

    const witness_schedule_object& wso = get_witness_schedule_object();
    db_impl().modify(witness, [&](witness_object& w) {
        auto delta_pos = w.votes.value * (wso.current_virtual_time - w.virtual_last_update);
        w.virtual_position += delta_pos;

        w.virtual_last_update = wso.current_virtual_time;
        w.votes += delta;
        FC_ASSERT(w.votes <= props.total_vesting_shares.amount, "",
                  ("w.votes", w.votes)("props", props.total_vesting_shares));

        w.virtual_scheduled_time
            = w.virtual_last_update + (VIRTUAL_SCHEDULE_LAP_LENGTH - w.virtual_position) / (w.votes.value + 1);

        /** witnesses with a low number of votes could overflow the time field and end up with a scheduled time in the
         * past */
        if (w.virtual_scheduled_time < wso.current_virtual_time)
            w.virtual_scheduled_time = fc::uint128::max_value();
    });
}
} // namespace chain
} // namespace scorum
