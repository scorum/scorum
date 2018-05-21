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
     * Returns all tags which where used sometime with specified category. The result is ordered by frequency of use.
     * @return <tag_name, frequency> pairs where 'tag_name' it's a tag which was used sometime with specified category
     * and
     * 'frequency' shows how many times this tag was used with specified category.
     */
    std::vector<std::pair<std::string, uint32_t>> get_tags_by_category(const std::string& category) const;

    /// @{ tags API
    std::vector<api::discussion> get_discussions_by_trending(const api::discussion_query& query) const;
    std::vector<api::discussion> get_discussions_by_created(const api::discussion_query& query) const;
    std::vector<api::discussion> get_discussions_by_hot(const api::discussion_query& query) const;
    /// @}

    api::discussion get_content(const std::string& author, const std::string& permlink) const;

    std::vector<api::discussion> get_comments(const std::string& parent_author,
                                              const std::string& parent_permlink,
                                              uint32_t depth = SCORUM_MAX_COMMENT_DEPTH) const;

    /**
     * This method is used to fetch all posts by author that occur after start_permlink
     * with up to limit being returned.
     *
     * If start_permlink is empty then discussions are returned from the beginning. This
     * should allow easy pagination.
     */
    std::vector<api::discussion> get_discussions_by_author(const api::discussion_query& query) const;
};

} // namespace tags
} // namespace scorum

// clang-format off
FC_API(scorum::tags::tags_api,
       (get_trending_tags)
       (get_tags_used_by_author)
       (get_tags_by_category)

       (get_discussions_by_trending)
       (get_discussions_by_created)
       (get_discussions_by_hot)

       // content
       (get_content)
       (get_comments)
       (get_discussions_by_author))
// clang-format on
