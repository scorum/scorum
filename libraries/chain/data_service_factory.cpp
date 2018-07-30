#include <scorum/chain/data_service_factory.hpp>

#include <scorum/chain/database/database.hpp>

#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/account_blogging_statistic.hpp>
#include <scorum/chain/services/atomicswap.hpp>
#include <scorum/chain/services/budgets.hpp>
#include <scorum/chain/services/comment.hpp>
#include <scorum/chain/services/comment_statistic.hpp>
#include <scorum/chain/services/comment_vote.hpp>
#include <scorum/chain/services/decline_voting_rights_request.hpp>
#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/chain/services/hardfork_property.hpp>
#include <scorum/chain/services/escrow.hpp>
#include <scorum/chain/services/proposal.hpp>
#include <scorum/chain/services/proposal_executor.hpp>
#include <scorum/chain/services/registration_committee.hpp>
#include <scorum/chain/services/registration_pool.hpp>
#include <scorum/chain/services/reward_balancer.hpp>
#include <scorum/chain/services/reward_funds.hpp>
#include <scorum/chain/services/scorumpower_delegation.hpp>
#include <scorum/chain/services/withdraw_scorumpower_route.hpp>
#include <scorum/chain/services/withdraw_scorumpower_route_statistic.hpp>
#include <scorum/chain/services/withdraw_scorumpower.hpp>
#include <scorum/chain/services/witness.hpp>
#include <scorum/chain/services/witness_schedule.hpp>
#include <scorum/chain/services/witness_vote.hpp>
#include <scorum/chain/services/dev_pool.hpp>
#include <scorum/chain/services/development_committee.hpp>
#include <scorum/chain/services/genesis_state.hpp>
#include <scorum/chain/services/account_registration_bonus.hpp>
#include <scorum/chain/services/witness_reward_in_sp_migration.hpp>
#include <scorum/chain/services/blocks_story.hpp>
#include <scorum/chain/services/advertising_property.hpp>
#include <scorum/chain/services/betting_property.hpp>
#include <scorum/chain/services/betting_service.hpp>
#include <scorum/chain/services/game.hpp>

// clang-format off
DATA_SERVICE_FACTORY_IMPL(
        (account)
        (account_blogging_statistic)
        (account_registration_bonus)
        (atomicswap)
        (fund_budget)
        (post_budget)
        (banner_budget)
        (comment)
        (comment_statistic_scr)
        (comment_statistic_sp)
        (comment_vote)
        (decline_voting_rights_request)
        (dynamic_global_property)
        (hardfork_property)
        (escrow)
        (proposal)
        (proposal_executor)
        (registration_committee)
        (development_committee)
        (registration_pool)
        (content_reward_fund_scr)
        (content_reward_fund_sp)
        (content_fifa_world_cup_2018_bounty_reward_fund)
        (content_reward_scr)
        (voters_reward_scr)
        (voters_reward_sp)
        (scorumpower_delegation)
        (withdraw_scorumpower_route)
        (withdraw_scorumpower_route_statistic)
        (withdraw_scorumpower)
        (witness)
        (witness_schedule)
        (witness_vote)
        (dev_pool)
        (genesis_state)
        (witness_reward_in_sp_migration)
        (blocks_story)
        (advertising_property)
        (betting_property)
        (betting)
        (game)
        )
// clang-format on
