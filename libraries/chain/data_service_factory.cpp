#include <scorum/chain/data_service_factory.hpp>

#include <scorum/chain/database/database.hpp>

#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/atomicswap.hpp>
#include <scorum/chain/services/budget.hpp>
#include <scorum/chain/services/comment.hpp>
#include <scorum/chain/services/comment_vote.hpp>
#include <scorum/chain/services/decline_voting_rights_request.hpp>
#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/chain/services/hardfork_property.hpp>
#include <scorum/chain/services/escrow.hpp>
#include <scorum/chain/services/proposal.hpp>
#include <scorum/chain/services/proposal_executor.hpp>
#include <scorum/chain/services/registration_committee.hpp>
#include <scorum/chain/services/registration_pool.hpp>
#include <scorum/chain/services/reward.hpp>
#include <scorum/chain/services/reward_fund.hpp>
#include <scorum/chain/services/scorumpower_delegation.hpp>
#include <scorum/chain/services/withdraw_scorumpower_route.hpp>
#include <scorum/chain/services/withdraw_scorumpower_route_statistic.hpp>
#include <scorum/chain/services/withdraw_scorumpower.hpp>
#include <scorum/chain/services/witness.hpp>
#include <scorum/chain/services/witness_schedule.hpp>
#include <scorum/chain/services/witness_vote.hpp>
#include <scorum/chain/services/dev_pool.hpp>
#include <scorum/chain/services/development_committee.hpp>

// clang-format off
DATA_SERVICE_FACTORY_IMPL(
        (account)
        (atomicswap)
        (budget)
        (comment)
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
        (reward)
        (reward_fund)
        (scorumpower_delegation)
        (withdraw_scorumpower_route)
        (withdraw_scorumpower_route_statistic)
        (withdraw_scorumpower)
        (witness)
        (witness_schedule)
        (witness_vote)
        (dev_pool)
        )
// clang-format on
