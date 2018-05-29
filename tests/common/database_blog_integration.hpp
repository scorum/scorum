#pragma once

#include <scorum/protocol/operations.hpp>
#include "database_trx_integration.hpp"
#include <functional>

namespace database_fixture {

struct database_blog_integration_fixture : public database_trx_integration_fixture
{
    database_blog_integration_fixture();

    virtual ~database_blog_integration_fixture() = default;

    class vote_op
    {
    public:
        vote_op(database_blog_integration_fixture* fixture,
                const vote_operation& op,
                fc::ecc::private_key actor_private_key);
        vote_op& operator=(const vote_op&);

        vote_op& push();
        vote_op& push(fc::ecc::private_key key);

        vote_op& in_block();
        vote_op& in_block(uint32_t delay_sec);
        vote_op& in_block(fc::microseconds delay);

    private:
        database_blog_integration_fixture* fixture;
        fc::ecc::private_key actor_private_key;
        vote_operation my;
        bool is_pushed = false;
    };

    class comment_op
    {
    public:
        using operation_type = vote_operation;

        comment_op(database_blog_integration_fixture* fixture,
                   const comment_operation& op,
                   fc::ecc::private_key actor_private_key);
        comment_op& operator=(const comment_op&);

        std::string title() const;
        std::string body() const;
        std::string author() const;
        std::string permlink() const;
        fc::time_point_sec cashout_time() const;

        vote_op vote(Actor& voter, int16_t weight = 100);

        comment_op create_comment(Actor& actor);
        comment_op& set_title(const std::string& title);
        comment_op& set_body(const std::string& body);
        comment_op& set_author(const std::string& author);
        comment_op& set_permlink(const std::string& permlink);
        comment_op& set_json(const std::string& json_metadata);

        void remove();

        comment_op& push();
        comment_op& push(fc::ecc::private_key key);

        comment_op& in_block();
        comment_op& in_block_with_delay(fc::microseconds delay = SCORUM_MIN_ROOT_COMMENT_INTERVAL);
        comment_op& in_block(uint32_t delay_sec);
        comment_op& in_block(fc::microseconds delay);

    private:
        database_blog_integration_fixture* fixture;
        fc::ecc::private_key actor_private_key;
        comment_operation my;
        bool is_pushed = false;
    };

    comment_op create_post(Actor& actor);
    comment_op create_post(Actor& actor, const std::string& category);

private:
    std::string get_unique_permlink();
};
}
