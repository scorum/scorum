#pragma once

#include "database_trx_integration.hpp"

#include <scorum/chain/services/comment.hpp>
#include <scorum/chain/services/comment_vote.hpp>
#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/chain/services/account.hpp>

#include <scorum/chain/schema/comment_objects.hpp>
#include <scorum/chain/schema/dynamic_global_property_object.hpp>
#include <scorum/chain/schema/account_objects.hpp>

namespace database_fixture {

struct blogging_common_fixture : public database_trx_integration_fixture
{
    blogging_common_fixture();

    std::string create_next_post_permlink();
    std::string get_comment_permlink(const std::string& post_permlink);

    const comment_object& post(const Actor& author, const std::string& post_permlink);

    const comment_object&
    comment(const Actor& author_for_post, const std::string& post_permlink, const Actor& author_for_comment);

    const comment_vote_object&
    vote(const Actor& author, const std::string& permlink, const Actor& voter, int wight = 100);

    comment_service_i& comment_service;
    comment_vote_service_i& comment_vote_service;
    account_service_i& account_service;
    dynamic_global_property_service_i& dgp_service;
};

struct blogging_common_with_accounts_fixture : public blogging_common_fixture
{
    blogging_common_with_accounts_fixture();

    Actor alice;
    Actor bob;
    Actor sam;
    Actor simon;

    const int feed_amount = 99000;
};
}
