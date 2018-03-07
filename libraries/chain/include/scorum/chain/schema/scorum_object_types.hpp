#pragma once

#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/mem_fun.hpp>

#include <fc/shared_string.hpp>
#include <fc/shared_containers.hpp>
#include <fc/static_variant.hpp>

#include <chainbase/chainbase.hpp>

#include <scorum/protocol/types.hpp>
#include <scorum/protocol/asset.hpp>
#include <scorum/protocol/authority.hpp>

namespace scorum {
namespace chain {

using namespace boost::multi_index;

using fc::shared_multi_index_container;

using chainbase::object;
using chainbase::oid;

using scorum::protocol::account_name_type;
using scorum::protocol::block_id_type;
using scorum::protocol::chain_id_type;
using scorum::protocol::share_type;
using scorum::protocol::transaction_id_type;

struct by_id;

enum object_type
{
    account_authority_object_type,
    account_object_type,
    account_recovery_request_object_type,
    atomicswap_contract_object_type,
    block_stats_object_type,
    block_summary_object_type,
    budget_object_type,
    chain_property_object_type,
    change_recovery_account_request_object_type,
    comment_object_type,
    comment_vote_object_type,
    decline_voting_rights_request_object_type,
    dynamic_global_property_object_type,
    escrow_object_type,
    hardfork_property_object_type,
    operation_object_type,
    owner_authority_history_object_type,
    proposal_object_type,
    registration_committee_member_object_type,
    registration_pool_object_type,
    reward_fund_object_type,
    reward_pool_object_type,
    transaction_object_type,
    scorumpower_delegation_expiration_object_type,
    scorumpower_delegation_object_type,
    withdraw_scorumpower_route_object_type,
    withdraw_scorumpower_route_statistic_object_type,
    withdraw_scorumpower_object_type,
    witness_object_type,
    witness_schedule_object_type,
    witness_vote_object_type,
    dev_committee_object_type,
};

class account_authority_object;
class account_object;
class account_recovery_request_object;
class atomicswap_contract_object;
class block_stats_object;
class block_summary_object;
class budget_object;
class chain_property_object;
class change_recovery_account_request_object;
class comment_object;
class comment_vote_object;
class decline_voting_rights_request_object;
class dynamic_global_property_object;
class escrow_object;
class hardfork_property_object;
class operation_object;
class owner_authority_history_object;
class proposal_object;
class registration_committee_member_object;
class registration_pool_object;
class reward_fund_object;
class reward_pool_object;
class transaction_object;
class scorumpower_delegation_expiration_object;
class scorumpower_delegation_object;
class withdraw_scorumpower_route_object;
class withdraw_scorumpower_route_statistic_object;
class withdraw_scorumpower_object;
class witness_object;
class witness_schedule_object;
class witness_vote_object;
class dev_committee_object;

using account_authority_id_type = oid<account_authority_object>;
using account_id_type = oid<account_object>;
using account_recovery_request_id_type = oid<account_recovery_request_object>;
using atomicswap_contract_id_type = oid<atomicswap_contract_object>;
using block_stats_id_type = oid<block_stats_object>;
using block_summary_id_type = oid<block_summary_object>;
using budget_id_type = oid<budget_object>;
using chain_property_id_type = oid<chain_property_object>;
using change_recovery_account_request_id_type = oid<change_recovery_account_request_object>;
using comment_id_type = oid<comment_object>;
using comment_vote_id_type = oid<comment_vote_object>;
using decline_voting_rights_request_id_type = oid<decline_voting_rights_request_object>;
using dynamic_global_property_id_type = oid<dynamic_global_property_object>;
using escrow_id_type = oid<escrow_object>;
using hardfork_property_id_type = oid<hardfork_property_object>;
using operation_id_type = oid<operation_object>;
using owner_authority_history_id_type = oid<owner_authority_history_object>;
using proposal_id_type = oid<proposal_object>;
using registration_committee_member_id_type = oid<registration_committee_member_object>;
using registration_pool_id_type = oid<registration_pool_object>;
using reward_fund_id_type = oid<reward_fund_object>;
using reward_pool_id_type = oid<reward_pool_object>;
using transaction_object_id_type = oid<transaction_object>;
using scorumpower_delegation_expiration_id_type = oid<scorumpower_delegation_expiration_object>;
using scorumpower_delegation_id_type = oid<scorumpower_delegation_object>;
using withdraw_scorumpower_route_id_type = oid<withdraw_scorumpower_route_object>;
using withdraw_scorumpower_route_statistic_id_type = oid<withdraw_scorumpower_route_statistic_object>;
using withdraw_scorumpower_id_type = oid<withdraw_scorumpower_object>;
using witness_id_type = oid<witness_object>;
using witness_schedule_id_type = oid<witness_schedule_object>;
using witness_vote_id_type = oid<witness_vote_object>;
using dev_committee_id_type = oid<dev_committee_object>;

using withdrawable_id_type = fc::static_variant<account_id_type, dev_committee_id_type>;

enum bandwidth_type
{
    post, ///< Rate limiting posting reward eligibility over time
    forum, ///< Rate limiting for all forum related actions
    market ///< Rate limiting for all other actions
};
} // namespace chain
} // namespace scorum

// clang-format off

FC_REFLECT_ENUM(scorum::chain::object_type,
                (account_authority_object_type)
                (account_object_type)
                (account_recovery_request_object_type)
                (atomicswap_contract_object_type)
                (block_stats_object_type)
                (block_summary_object_type)
                (budget_object_type)
                (chain_property_object_type)
                (change_recovery_account_request_object_type)
                (comment_object_type)
                (comment_vote_object_type)
                (decline_voting_rights_request_object_type)
                (dynamic_global_property_object_type)
                (escrow_object_type)
                (hardfork_property_object_type)
                (operation_object_type)
                (owner_authority_history_object_type)
                (proposal_object_type)
                (registration_committee_member_object_type)
                (registration_pool_object_type)
                (reward_fund_object_type)
                (reward_pool_object_type)
                (transaction_object_type)
                (scorumpower_delegation_expiration_object_type)
                (scorumpower_delegation_object_type)
                (withdraw_scorumpower_route_object_type)
                (withdraw_scorumpower_route_statistic_object_type)
                (withdraw_scorumpower_object_type)
                (witness_object_type)
                (witness_schedule_object_type)
                (witness_vote_object_type)
                (dev_committee_object_type)
               )

FC_REFLECT_ENUM( scorum::chain::bandwidth_type, (post)(forum)(market) )

// clang-format on
