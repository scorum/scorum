#pragma once

#include <memory>

#include <scorum/protocol/types.hpp>
#include <scorum/tags/tags_api_objects.hpp>

#define TAGS_API_NAME "tags_api"

namespace chainbase {
class database_guard;
}

namespace scorum {
namespace tags {

class tags_api_impl;

class tags_api : public std::enable_shared_from_this<tags_api>
{
    std::unique_ptr<tags_api_impl> _impl;

    std::shared_ptr<chainbase::database_guard> _guard;

    chainbase::database_guard& guard() const;

public:
    tags_api(const app::api_context& ctx);
    ~tags_api();

    void on_api_startup();

    std::vector<api::tag_api_obj> get_trending_tags(const std::string& after_tag, uint32_t limit) const;

    std::vector<std::pair<std::string, uint32_t>> get_tags_used_by_author(const std::string& author) const;

    /**
     * @brief get_tags_by_category
     * @return
     */
    std::vector<std::pair<std::string, uint32_t>> get_tags_by_category(const std::string& category) const;

    /// @{ tags API
    /// This API will return the top 1000 tags used by an author sorted by most frequently used
    std::vector<api::discussion> get_discussions_by_payout(const api::discussion_query& query) const;
    std::vector<api::discussion> get_post_discussions_by_payout(const api::discussion_query& query) const;
    std::vector<api::discussion> get_comment_discussions_by_payout(const api::discussion_query& query) const;
    std::vector<api::discussion> get_discussions_by_trending(const api::discussion_query& query) const;
    std::vector<api::discussion> get_discussions_by_created(const api::discussion_query& query) const;
    std::vector<api::discussion> get_discussions_by_hot(const api::discussion_query& query) const;
    std::vector<api::discussion> get_discussions_by_promoted(const api::discussion_query& query) const;
    std::vector<api::discussion> get_discussions_by_active(const api::discussion_query& query) const;
    std::vector<api::discussion> get_discussions_by_cashout(const api::discussion_query& query) const;
    std::vector<api::discussion> get_discussions_by_votes(const api::discussion_query& query) const;
    std::vector<api::discussion> get_discussions_by_children(const api::discussion_query& query) const;
    std::vector<api::discussion> get_discussions_by_comments(const api::discussion_query& query) const;
    /// @}

    api::discussion get_content(const std::string& author, const std::string& permlink) const;
    std::vector<api::discussion> get_content_replies(const std::string& parent,
                                                     const std::string& parent_permlink) const;

    /**
     *  Return the active discussions with the highest cumulative pending payouts without respect to category, total
     *  pending payout means the pending payout of all children as well.
     *  This method can be used to fetch replies to an account.
     *
     *  The first call should be (account_to_retrieve replies, "", limit)
     *  Subsequent calls should be (last_author, last_permlink, limit)
     */
    std::vector<api::discussion>
    get_replies_by_last_update(account_name_type start_author, const std::string& start_permlink, uint32_t limit) const;

    /**
     * This method is used to fetch all posts/comments by start_author that occur after before_date and start_permlink
     * with up to limit being returned.
     *
     * If start_permlink is empty then only before_date will be considered. If both are specified the earlier to the
     * two metrics will be used. This
     * should allow easy pagination.
     */
    std::vector<api::discussion> get_discussions_by_author_before_date(const std::string& author,
                                                                       const std::string& start_permlink,
                                                                       time_point_sec before_date,
                                                                       uint32_t limit) const;

    /**
     *  This API is a short-cut for returning all of the state required for a particular URL
     *  with a single query.
     */
    scorum::tags::api::state get_state(std::string path) const;
};

} // namespace tags
} // namespace scorum

// clang-format off
FC_API(scorum::tags::tags_api,
       (get_trending_tags)
       (get_tags_used_by_author)
       (get_tags_by_category)
       (get_discussions_by_payout)
       (get_post_discussions_by_payout)
       (get_comment_discussions_by_payout)
       (get_discussions_by_trending)
       (get_discussions_by_created)
       (get_discussions_by_hot)
       (get_discussions_by_promoted)
       (get_discussions_by_active)
       (get_discussions_by_cashout)
       (get_discussions_by_votes)
       (get_discussions_by_children)
       (get_discussions_by_comments)

       (get_state)

       // content
       (get_content)
       (get_content_replies)
       (get_discussions_by_author_before_date)
       (get_replies_by_last_update))
// clang-format on
