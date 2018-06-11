#include <scorum/chain/services/comment.hpp>
#include "database_blog_integration.hpp"
#include <fc/time.hpp>

namespace database_fixture {

database_blog_integration_fixture::database_blog_integration_fixture()
{
}

database_blog_integration_fixture::comment_op database_blog_integration_fixture::create_post(Actor& actor)
{
    return create_post(actor, actor.name + "-category");
}

database_blog_integration_fixture::comment_op
database_blog_integration_fixture::create_post(Actor& actor, const std::string& category)
{
    comment_operation op;
    op.author = actor.name;
    op.parent_author = SCORUM_ROOT_POST_PARENT_ACCOUNT;
    op.permlink = get_unique_permlink();
    op.body = actor.name + "-body";
    op.title = actor.name + "-title";
    op.parent_permlink = category;

    return comment_op(this, op, actor.private_key);
}

std::string database_blog_integration_fixture::get_unique_permlink()
{
    static uint32_t permlink_no = 0;
    permlink_no++;

    return boost::lexical_cast<std::string>(permlink_no);
}

database_blog_integration_fixture::comment_op::comment_op(database_blog_integration_fixture* fixture,
                                                          const comment_operation& op,
                                                          fc::ecc::private_key actor_private_key)
    : fixture(fixture)
    , actor_private_key(actor_private_key)
    , my(op)
{
}

database_blog_integration_fixture::comment_op& database_blog_integration_fixture::comment_op::
operator=(const comment_op& c)
{
    this->fixture = c.fixture;
    this->my = c.my;
    this->actor_private_key = c.actor_private_key;

    return *this;
}

std::string database_blog_integration_fixture::comment_op::title() const
{
    return my.title;
}

std::string database_blog_integration_fixture::comment_op::body() const
{
    return my.body;
}

std::string database_blog_integration_fixture::comment_op::author() const
{
    return my.author;
}

std::string database_blog_integration_fixture::comment_op::permlink() const
{
    return my.permlink;
}

fc::time_point_sec database_blog_integration_fixture::comment_op::cashout_time() const
{
    const auto& comment = fixture->services.comment_service().get(author(), permlink());

    return comment.cashout_time;
}

database_blog_integration_fixture::comment_op
database_blog_integration_fixture::comment_op::create_comment(Actor& actor)
{
    comment_operation op;
    op.author = actor.name;
    op.permlink = fixture->get_unique_permlink();
    op.parent_author = my.author;
    op.parent_permlink = my.permlink;
    op.body = actor.name + "-body";
    op.title = actor.name + "-title";

    return comment_op(fixture, op, actor.private_key);
}

database_blog_integration_fixture::vote_op database_blog_integration_fixture::comment_op::vote(Actor& voter,
                                                                                               int16_t weight)
{
    vote_operation op;
    op.author = author();
    op.permlink = permlink();
    op.voter = voter.name;
    op.weight = weight;

    return vote_op(fixture, op, voter.private_key);
}

database_blog_integration_fixture::comment_op&
database_blog_integration_fixture::comment_op::set_title(const std::string& title)
{
    this->my.title = title;
    return *this;
}

database_blog_integration_fixture::comment_op&
database_blog_integration_fixture::comment_op::set_body(const std::string& body)
{
    this->my.body = body;
    return *this;
}

database_blog_integration_fixture::comment_op&
database_blog_integration_fixture::comment_op::set_author(const std::string& author)
{
    this->my.author = author;
    return *this;
}

database_blog_integration_fixture::comment_op&
database_blog_integration_fixture::comment_op::set_permlink(const std::string& permlink)
{
    this->my.permlink = permlink;
    return *this;
}

database_blog_integration_fixture::comment_op&
database_blog_integration_fixture::comment_op::set_json(const std::string& json_metadata)
{
    this->my.json_metadata = json_metadata;
    return *this;
}

void database_blog_integration_fixture::comment_op::remove()
{
    delete_comment_operation op;
    op.author = this->author();
    op.permlink = this->permlink();

    fixture->push_operation(op, this->actor_private_key, true);
}

database_blog_integration_fixture::comment_op& database_blog_integration_fixture::comment_op::push()
{
    return push(actor_private_key);
}

database_blog_integration_fixture::comment_op&
database_blog_integration_fixture::comment_op::push(fc::ecc::private_key key)
{
    is_pushed = true;
    fixture->push_operation(this->my, key, false);
    return *this;
}

database_blog_integration_fixture::comment_op& database_blog_integration_fixture::comment_op::in_block()
{
    if (!is_pushed)
        push();

    fixture->generate_block();
    return *this;
}

database_blog_integration_fixture::comment_op&
database_blog_integration_fixture::comment_op::in_block(uint32_t delay_sec)
{
    return in_block(fc::seconds(delay_sec));
}

database_blog_integration_fixture::comment_op&
database_blog_integration_fixture::comment_op::in_block_with_delay(fc::microseconds delay)
{
    return in_block(delay);
}

database_blog_integration_fixture::comment_op&
database_blog_integration_fixture::comment_op::in_block(fc::microseconds delay)
{
    if (!is_pushed)
        push();

    fixture->generate_blocks(fixture->db.head_block_time() + delay, true);

    return *this;
}

database_blog_integration_fixture::vote_op::vote_op(database_blog_integration_fixture* fixture,
                                                    const vote_operation& op,
                                                    fc::ecc::private_key actor_private_key)
    : fixture(fixture)
    , actor_private_key(actor_private_key)
    , my(op)
{
}

database_blog_integration_fixture::vote_op& database_blog_integration_fixture::vote_op::operator=(const vote_op& v)
{
    this->fixture = v.fixture;
    this->my = v.my;
    this->actor_private_key = v.actor_private_key;

    return *this;
}

database_blog_integration_fixture::vote_op& database_blog_integration_fixture::vote_op::push()
{
    is_pushed = true;
    fixture->push_operation(my, actor_private_key, false);

    return *this;
}

database_blog_integration_fixture::vote_op& database_blog_integration_fixture::vote_op::in_block(fc::microseconds delay)
{
    if (!is_pushed)
        push();

    fixture->generate_blocks(fixture->db.head_block_time() + delay, true);

    return *this;
}

database_blog_integration_fixture::vote_op& database_blog_integration_fixture::vote_op::in_block()
{
    if (!is_pushed)
        push();

    fixture->generate_block();

    return *this;
}
}
