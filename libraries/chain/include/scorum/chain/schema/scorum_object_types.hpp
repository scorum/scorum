#pragma once

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/mem_fun.hpp>

#include <fc/shared_string.hpp>
#include <chainbase/chainbase.hpp>

#include <scorum/protocol/types.hpp>
#include <scorum/protocol/asset.hpp>
#include <scorum/protocol/authority.hpp>

namespace scorum {
namespace chain {

using namespace boost::multi_index;

using boost::multi_index_container;

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
    dynamic_global_property_object_type,
    chain_property_object_type,
    account_object_type,
    account_authority_object_type,
    witness_object_type,
    transaction_object_type,
    block_summary_object_type,
    witness_schedule_object_type,
    comment_object_type,
    comment_vote_object_type,
    witness_vote_object_type,
    operation_object_type,
    account_history_object_type,
    hardfork_property_object_type,
    withdraw_vesting_route_object_type,
    owner_authority_history_object_type,
    account_recovery_request_object_type,
    change_recovery_account_request_object_type,
    escrow_object_type,
    decline_voting_rights_request_object_type,
    block_stats_object_type,
    reward_fund_object_type,
    reward_pool_object_type,
    vesting_delegation_object_type,
    vesting_delegation_expiration_object_type,
    budget_object_type,
    registration_pool_object_type,
    registration_committee_member_object_type,
    atomicswap_contract_object_type,
    proposal_object_type
};

class dynamic_global_property_object;
class chain_property_object;
class account_object;
class account_authority_object;
class witness_object;
class transaction_object;
class block_summary_object;
class witness_schedule_object;
class comment_object;
class comment_vote_object;
class witness_vote_object;
class operation_object;
class account_history_object;
class hardfork_property_object;
class withdraw_vesting_route_object;
class owner_authority_history_object;
class account_recovery_request_object;
class change_recovery_account_request_object;
class escrow_object;
class decline_voting_rights_request_object;
class block_stats_object;
class reward_fund_object;
class reward_pool_object;
class vesting_delegation_object;
class vesting_delegation_expiration_object;
class budget_object;
class registration_pool_object;
class registration_committee_member_object;
class atomicswap_contract_object;
class proposal_object;

typedef oid<dynamic_global_property_object> dynamic_global_property_id_type;
typedef oid<chain_property_object> chain_property_id_type;
typedef oid<account_object> account_id_type;
typedef oid<account_authority_object> account_authority_id_type;
typedef oid<witness_object> witness_id_type;
typedef oid<transaction_object> transaction_object_id_type;
typedef oid<block_summary_object> block_summary_id_type;
typedef oid<witness_schedule_object> witness_schedule_id_type;
typedef oid<comment_object> comment_id_type;
typedef oid<comment_vote_object> comment_vote_id_type;
typedef oid<witness_vote_object> witness_vote_id_type;
typedef oid<operation_object> operation_id_type;
typedef oid<account_history_object> account_history_id_type;
typedef oid<hardfork_property_object> hardfork_property_id_type;
typedef oid<withdraw_vesting_route_object> withdraw_vesting_route_id_type;
typedef oid<owner_authority_history_object> owner_authority_history_id_type;
typedef oid<account_recovery_request_object> account_recovery_request_id_type;
typedef oid<change_recovery_account_request_object> change_recovery_account_request_id_type;
typedef oid<escrow_object> escrow_id_type;
typedef oid<decline_voting_rights_request_object> decline_voting_rights_request_id_type;
typedef oid<block_stats_object> block_stats_id_type;
typedef oid<reward_fund_object> reward_fund_id_type;
typedef oid<reward_pool_object> reward_pool_id_type;
typedef oid<vesting_delegation_object> vesting_delegation_id_type;
typedef oid<vesting_delegation_expiration_object> vesting_delegation_expiration_id_type;
typedef oid<budget_object> budget_id_type;
typedef oid<registration_pool_object> registration_pool_id_type;
typedef oid<registration_committee_member_object> registration_committee_member_id_type;
typedef oid<atomicswap_contract_object> atomicswap_contract_id_type;
typedef oid<proposal_object> proposal_id_type;

enum bandwidth_type
{
    post, ///< Rate limiting posting reward eligibility over time
    forum, ///< Rate limiting for all forum related actions
    market ///< Rate limiting for all other actions
};
} // namespace chain
} // namespace scorum

// clang-format off

FC_REFLECT_ENUM( scorum::chain::object_type,
                 (dynamic_global_property_object_type)
                 (chain_property_object_type)
                 (account_object_type)
                 (account_authority_object_type)
                 (witness_object_type)
                 (transaction_object_type)
                 (block_summary_object_type)
                 (witness_schedule_object_type)
                 (comment_object_type)
                 (comment_vote_object_type)
                 (witness_vote_object_type)
                 (operation_object_type)
                 (account_history_object_type)
                 (hardfork_property_object_type)
                 (withdraw_vesting_route_object_type)
                 (owner_authority_history_object_type)
                 (account_recovery_request_object_type)
                 (change_recovery_account_request_object_type)
                 (escrow_object_type)
                 (decline_voting_rights_request_object_type)
                 (block_stats_object_type)
                 (reward_fund_object_type)
                 (reward_pool_object_type)
                 (vesting_delegation_object_type)
                 (vesting_delegation_expiration_object_type)
                 (budget_object_type)
                 (registration_pool_object_type)
                 (registration_committee_member_object_type)
                 (atomicswap_contract_object_type)
                 (proposal_object_type)
                 )

FC_REFLECT_ENUM( scorum::chain::bandwidth_type, (post)(forum)(market) )

// clang-format on
