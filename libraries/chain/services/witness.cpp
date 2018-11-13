#include <scorum/chain/services/witness.hpp>
#include <scorum/chain/services/witness_schedule.hpp>
#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/chain/dba/db_accessor.hpp>
#include <scorum/chain/database/database.hpp>

#include <scorum/chain/schema/account_objects.hpp>
#include <scorum/chain/schema/witness_objects.hpp>
#include <scorum/chain/schema/chain_property_object.hpp>

#include <scorum/protocol/scorum_operations.hpp>

namespace scorum {
namespace chain {

dbs_witness::dbs_witness(dba::db_index& db,
                         witness_schedule_service_i& witness_schedule_svc,
                         dynamic_global_property_service_i& dgp_svc,
                         dba::db_accessor<chain_property_object>& chain_dba)
    : base_service_type(db)
    , _dgp_svc(dgp_svc)
    , _witness_schedule_svc(witness_schedule_svc)
    , _chain_dba(chain_dba)
{
}

const witness_object& dbs_witness::get(const account_name_type& name) const
{
    try
    {
        return get_by<by_name>(name);
    }
    FC_CAPTURE_AND_RETHROW((name))
}

bool dbs_witness::is_exists(const account_name_type& name) const
{
    return find_by<by_name>(name) != nullptr;
}

const witness_object& dbs_witness::get_top_witness() const
{
    const auto& idx = db_impl().get_index<witness_index>().indices().get<by_vote_name>();
    FC_ASSERT(idx.begin() != idx.end(), "Empty witness_index by_vote_name.");
    return (*idx.begin());
}

const witness_object& dbs_witness::create_witness(const account_name_type& owner,
                                                  const std::string& url,
                                                  const public_key_type& block_signing_key,
                                                  const chain_properties& props)
{
    FC_ASSERT(owner.size(), "Witness 'owner_name' should not be empty.");

    const auto& new_witness = create_internal(owner, block_signing_key);

    update(new_witness, [&](witness_object& w) {
        fc::from_string(w.url, url);
        w.created = _dgp_svc.head_block_time();
        w.proposed_chain_props = props;
    });

    return new_witness;
}

const witness_object& dbs_witness::create_initial_witness(const account_name_type& owner,
                                                          const public_key_type& block_signing_key)
{
    const auto& new_witness = create_internal(owner, block_signing_key);

    update(new_witness, [&](witness_object& w) { w.schedule = witness_object::top20; });

    return new_witness;
}

const witness_object& dbs_witness::create_internal(const account_name_type& owner,
                                                   const public_key_type& block_signing_key)
{
    return create([&](witness_object& w) {
        w.owner = owner;
        w.signing_key = block_signing_key;
        w.hardfork_time_vote = _chain_dba.get().genesis_time;
    });
}

void dbs_witness::update_witness(const witness_object& witness,
                                 const std::string& url,
                                 const public_key_type& block_signing_key,
                                 const chain_properties& props)
{
    update(witness, [&](witness_object& w) {
        fc::from_string(w.url, url);
        w.signing_key = block_signing_key;
        w.proposed_chain_props = props;
    });
}

void dbs_witness::adjust_witness_votes(const account_object& account, const share_type& delta)
{
    const auto& vidx = db_impl().get_index<witness_vote_index>().indices().get<by_account_witness>();
    auto itr = vidx.lower_bound(boost::make_tuple(account.id, witness_id_type()));
    while (itr != vidx.end() && itr->account == account.id)
    {
        adjust_witness_vote(db_impl().get(itr->witness), delta);
        ++itr;
    }
}

void dbs_witness::adjust_witness_vote(const witness_object& witness, const share_type& delta)
{
    block_info ctx = get_head_block_context();

    const auto& props = _dgp_svc.get();
    const auto& wso = _witness_schedule_svc.get();

    update(witness, [&](witness_object& w) {
        debug_log(ctx, "updating votes for witness=${w}", ("w", w.owner));

        auto delta_pos = w.votes.value * (wso.current_virtual_time - w.virtual_last_update);
        w.virtual_position += delta_pos;

        w.virtual_last_update = wso.current_virtual_time;

        debug_log(ctx, "old_votes=${v}", ("v", w.votes));

        w.votes += delta;
        FC_ASSERT(w.votes <= props.total_scorumpower.amount, "",
                  ("w.votes", w.votes)("props", props.total_scorumpower));

        w.virtual_scheduled_time
            = w.virtual_last_update + (VIRTUAL_SCHEDULE_LAP_LENGTH - w.virtual_position) / (w.votes.value + 1);

        /** witnesses with a low number of votes could overflow the time field and end up with a scheduled time in the
         * past */
        if (w.virtual_scheduled_time < wso.current_virtual_time)
            w.virtual_scheduled_time = fc::uint128::max_value();

        debug_log(ctx, "new_votes=${v}", ("v", w.votes));
    });
}

block_info dbs_witness::get_head_block_context()
{
    const auto& dprop = _dgp_svc.get();
    block_info ctx(dprop.head_block_number, dprop.head_block_id.str(), dprop.time, dprop.current_witness);

    return ctx;
}
} // namespace chain
} // namespace scorum
