#pragma once

#include <scorum/app/application.hpp>
#include <scorum/app/scorum_api_objects.hpp>

#include <scorum/follow/follow_objects.hpp>

#include <fc/api.hpp>

namespace scorum {
namespace follow {

using app::comment_api_obj;

struct feed_entry
{
    std::string author;
    std::string permlink;
    std::vector<account_name_type> reblog_by;
    time_point_sec reblog_on;
    uint32_t entry_id = 0;
};

struct comment_feed_entry
{
    comment_api_obj comment;
    std::vector<account_name_type> reblog_by;
    time_point_sec reblog_on;
    uint32_t entry_id = 0;
};

struct blog_entry
{
    std::string author;
    std::string permlink;
    std::string blog;
    time_point_sec reblog_on;
    uint32_t entry_id = 0;
};

struct comment_blog_entry
{
    comment_api_obj comment;
    std::string blog;
    time_point_sec reblog_on;
    uint32_t entry_id = 0;
};

struct account_reputation
{
    std::string account;
    share_type reputation;
};

struct follow_api_obj
{
    std::string follower;
    std::string following;
    std::vector<follow_type> what;
};

struct follow_count_api_obj
{
    follow_count_api_obj()
    {
    }
    follow_count_api_obj(const follow_count_object& o)
        : account(o.account)
        , follower_count(o.follower_count)
        , following_count(o.following_count)
    {
    }

    std::string account;
    uint32_t follower_count = 0;
    uint32_t following_count = 0;
};

namespace detail {
class follow_api_impl;
}

class follow_api
{
public:
    follow_api(const app::api_context& ctx);

    void on_api_startup();

    std::vector<follow_api_obj>
    get_followers(std::string to, std::string start, follow_type type, uint16_t limit) const;
    std::vector<follow_api_obj>
    get_following(std::string from, std::string start, follow_type type, uint16_t limit) const;

    follow_count_api_obj get_follow_count(std::string account) const;

    std::vector<feed_entry> get_feed_entries(std::string account, uint32_t entry_id = 0, uint16_t limit = 500) const;
    std::vector<comment_feed_entry> get_feed(std::string account, uint32_t entry_id = 0, uint16_t limit = 500) const;

    std::vector<blog_entry> get_blog_entries(std::string account, uint32_t entry_id = 0, uint16_t limit = 500) const;
    std::vector<comment_blog_entry> get_blog(std::string account, uint32_t entry_id = 0, uint16_t limit = 500) const;

    std::vector<account_reputation> get_account_reputations(std::string lower_bound_name, uint32_t limit = 1000) const;

    /**
     * Gets list of accounts that have reblogged a particular post
     */
    std::vector<account_name_type> get_reblogged_by(const std::string& author, const std::string& permlink) const;

    /**
     * Gets a list of authors that have had their content reblogged on a given blog account
     */
    std::vector<std::pair<account_name_type, uint32_t>> get_blog_authors(const account_name_type& blog_account) const;

private:
    std::shared_ptr<detail::follow_api_impl> my;
};
}
} // scorum::follow

FC_REFLECT(scorum::follow::feed_entry, (author)(permlink)(reblog_by)(reblog_on)(entry_id))
FC_REFLECT(scorum::follow::comment_feed_entry, (comment)(reblog_by)(reblog_on)(entry_id))
FC_REFLECT(scorum::follow::blog_entry, (author)(permlink)(blog)(reblog_on)(entry_id))
FC_REFLECT(scorum::follow::comment_blog_entry, (comment)(blog)(reblog_on)(entry_id))
FC_REFLECT(scorum::follow::account_reputation, (account)(reputation))
FC_REFLECT(scorum::follow::follow_api_obj, (follower)(following)(what))
FC_REFLECT(scorum::follow::follow_count_api_obj, (account)(follower_count)(following_count))

FC_API(scorum::follow::follow_api,
       (get_followers)(get_following)(get_follow_count)(get_feed_entries)(get_feed)(get_blog_entries)(get_blog)(
           get_account_reputations)(get_reblogged_by)(get_blog_authors))
