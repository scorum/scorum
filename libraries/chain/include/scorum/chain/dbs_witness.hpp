#pragma once

#include <scorum/chain/dbs_base_impl.hpp>

#include <scorum/chain/witness_objects.hpp>

namespace scorum {
namespace chain {

class account_object;

class dbs_witness : public dbs_base
{
    friend class dbservice_dbs_factory;

protected:
    explicit dbs_witness(database& db);

public:
    const witness_object& get(const account_name_type& owner) const;

    bool is_exists(const account_name_type& owner) const;

    const witness_schedule_object& get_witness_schedule_object() const;

    const witness_object& get_top_witness() const;

    const witness_object& create_witness(const account_name_type& owner,
                                         const std::string& url,
                                         const public_key_type& block_signing_key,
                                         const chain_properties& props);

    void update_witness(const witness_object& witness,
                        const std::string& url,
                        const public_key_type& block_signing_key,
                        const chain_properties& props);

    /** this updates the vote of a single witness as a result of a vote being added or removed*/
    void adjust_witness_vote(const witness_object& witness, const share_type& delta);

    /** this is called by `adjust_proxied_witness_votes` when account proxy to self */
    void adjust_witness_votes(const account_object& account, const share_type& delta);
};
} // namespace chain
} // namespace scorum
