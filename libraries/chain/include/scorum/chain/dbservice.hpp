#pragma once

#include <fc/shared_string.hpp>
#include <scorum/chain/custom_operation_interpreter.hpp>

#include <scorum/chain/dbs_base_impl.hpp>

namespace chainbase {
class database; // for _temporary_public_imp only
}

namespace scorum {
namespace chain {

class dbservice : public dbservice_dbs_factory
{
    typedef dbservice_dbs_factory _base_type;

protected:
    explicit dbservice(database&);

public:
    virtual ~dbservice();

    // TODO: These methods have copied from database public methods.
    //       Most of these methods will be moved to dbs specific services

    virtual bool is_producing() const = 0;

    virtual const witness_object& get_witness(const account_name_type& name) const = 0;

    virtual const account_object& get_account(const account_name_type& name) const = 0;

    virtual const comment_object& get_comment(const account_name_type& author,
                                              const fc::shared_string& permlink) const = 0;

    virtual const comment_object& get_comment(const account_name_type& author, const std::string& permlink) const = 0;

    virtual const escrow_object& get_escrow(const account_name_type& name, uint32_t escrow_id) const = 0;

    virtual asset get_balance(const account_object& a, asset_symbol_type symbol) const = 0;

    virtual asset get_balance(const std::string& aname, asset_symbol_type symbol) const = 0;

    virtual const dynamic_global_property_object& get_dynamic_global_properties() const = 0;

    virtual const witness_schedule_object& get_witness_schedule_object() const = 0;

    virtual uint16_t get_curation_rewards_percent(const comment_object& c) const = 0;

    virtual const reward_fund_object& get_reward_fund(const comment_object& c) const = 0;

    virtual time_point_sec head_block_time() const = 0;

    virtual const time_point_sec calculate_discussion_payout_time(const comment_object& comment) const = 0;

    virtual std::shared_ptr<custom_operation_interpreter> get_custom_json_evaluator(const std::string& id) = 0;

    virtual fc::time_point_sec get_genesis_time() const = 0;

    // for TODO only:
    chainbase::database& _temporary_public_impl();
};
}
}
