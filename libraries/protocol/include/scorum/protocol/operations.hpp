#pragma once

#include <scorum/protocol/operation_util.hpp>
#include <scorum/protocol/scorum_operations.hpp>
#include <scorum/protocol/scorum_virtual_operations.hpp>

namespace scorum {
namespace protocol {

/** NOTE: do not change the order of any operations prior to the virtual operations
 * or it will trigger a hardfork.
 */
typedef fc::static_variant<vote_operation,
                           comment_operation,

                           transfer_operation,
                           transfer_to_vesting_operation,
                           withdraw_vesting_operation,

                           account_create_operation,
                           account_update_operation,

                           witness_update_operation,
                           account_witness_vote_operation,
                           account_witness_proxy_operation,

                           custom_operation,

                           delete_comment_operation,
                           custom_json_operation,
                           comment_options_operation,
                           set_withdraw_vesting_route_operation,

                           prove_authority_operation,

                           request_account_recovery_operation,
                           recover_account_operation,
                           change_recovery_account_operation,
                           escrow_transfer_operation,
                           escrow_dispute_operation,
                           escrow_release_operation,
                           escrow_approve_operation,

                           custom_binary_operation,
                           decline_voting_rights_operation,
                           claim_reward_balance_operation,
                           delegate_vesting_shares_operation,
                           account_create_with_delegation_operation,

                           create_budget_operation,
                           close_budget_operation,

                           vote_for_registration_committee_proposal_operation,

                           /// virtual operations
                           author_reward_operation,
                           curation_reward_operation,
                           comment_reward_operation,
                           fill_vesting_withdraw_operation,
                           shutdown_witness_operation,
                           hardfork_operation,
                           comment_payout_update_operation,
                           return_vesting_delegation_operation,
                           comment_benefactor_reward_operation,
                           producer_reward_operation>
    operation;

/*void operation_get_required_authorities( const operation& op,
                                         flat_set<string>& active,
                                         flat_set<string>& owner,
                                         flat_set<string>& posting,
                                         vector<authority>&  other );

void operation_validate( const operation& op );*/

bool is_market_operation(const operation& op);

bool is_virtual_operation(const operation& op);
} // namespace protocol
} // namespace scorum

/*namespace fc {
    void to_variant( const scorum::protocol::operation& var,  fc::variant& vo );
    void from_variant( const fc::variant& var,  scorum::protocol::operation& vo );
}*/

DECLARE_OPERATION_TYPE(scorum::protocol::operation)
FC_REFLECT_TYPENAME(scorum::protocol::operation)
