#pragma once

#include <scorum/chain/services/base_service.hpp>

namespace scorum {
namespace protocol {
class chain_properties;
}
namespace chain {

class account_object;
class witness_object;
class witness_schedule_object;

using chain_properties = scorum::protocol::chain_properties;

struct witness_service_i
{
    virtual const witness_object& get(const account_name_type& owner) const = 0;

    virtual bool is_exists(const account_name_type& owner) const = 0;

    virtual const witness_schedule_object& get_witness_schedule_object() const = 0;

    virtual const witness_object& get_top_witness() const = 0;

    virtual const witness_object& create_witness(const account_name_type& owner,
                                                 const std::string& url,
                                                 const public_key_type& block_signing_key,
                                                 const chain_properties& props)
        = 0;

    virtual void update_witness(const witness_object& witness,
                                const std::string& url,
                                const public_key_type& block_signing_key,
                                const chain_properties& props)
        = 0;

    /** this updates the vote of a single witness as a result of a vote being added or removed*/
    virtual void adjust_witness_vote(const witness_object& witness, const share_type& delta) = 0;

    /** this is called by `adjust_proxied_witness_votes` when account proxy to self */
    virtual void adjust_witness_votes(const account_object& account, const share_type& delta) = 0;
};

class dbs_witness : public dbs_base, public witness_service_i
{
    friend class dbservice_dbs_factory;

protected:
    explicit dbs_witness(database& db);

public:
    const witness_object& get(const account_name_type& owner) const override;

    bool is_exists(const account_name_type& owner) const override;

    const witness_schedule_object& get_witness_schedule_object() const override;

    const witness_object& get_top_witness() const override;

    const witness_object& create_witness(const account_name_type& owner,
                                         const std::string& url,
                                         const public_key_type& block_signing_key,
                                         const chain_properties& props) override;

    void update_witness(const witness_object& witness,
                        const std::string& url,
                        const public_key_type& block_signing_key,
                        const chain_properties& props) override;

    /** this updates the vote of a single witness as a result of a vote being added or removed*/
    void adjust_witness_vote(const witness_object& witness, const share_type& delta) override;

    /** this is called by `adjust_proxied_witness_votes` when account proxy to self */
    void adjust_witness_votes(const account_object& account, const share_type& delta) override;
};
} // namespace chain
} // namespace scorum
