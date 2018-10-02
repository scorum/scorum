#pragma once

#include <scorum/chain/data_service_factory_def.hpp>

// clang-format off
DATA_SERVICE_FACTORY_DECLARE(
        (account)
        (account_blogging_statistic)
        (account_registration_bonus)
        (atomicswap)
        (fund_budget)
//        (post_budget)
//        (banner_budget)
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
        )
// clang-format on

namespace scorum {
namespace chain {

class database;
class post_budget_service_i;
class banner_budget_service_i;

class data_service_factory_i : virtual public data_service_factory_i_impl
{
public:
    ~data_service_factory_i()
    {
    }

    virtual post_budget_service_i& post_budget_service() = 0;
    virtual banner_budget_service_i& banner_budget_service() = 0;
};

class data_service_factory : public data_service_factory_i, public data_service_factory_impl
{
public:
    data_service_factory(database& db)
        : data_service_factory_impl(db)
        , _db(db)
    {
    }

    ~data_service_factory()
    {
    }

    post_budget_service_i& post_budget_service() override;
    banner_budget_service_i& banner_budget_service() override;

private:
    database& _db;
};

} // namespace chain
} // namespace scorum
