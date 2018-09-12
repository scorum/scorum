#include <scorum/tags/tags_api.hpp>

#include <scorum/tags/tags_api_impl.hpp>

namespace scorum {
namespace tags {

using namespace scorum::chain;
using namespace scorum::protocol;
using namespace scorum::tags::api;

chainbase::database_guard& tags_api::guard() const
{
    return *_guard;
}

tags_api::tags_api(const app::api_context& ctx)
    : _impl(new tags_api_impl(*ctx.app.chain_database()))
    , _guard(ctx.app.chain_database())
{
}

tags_api::~tags_api()
{
}

void tags_api::on_api_startup()
{
}

std::vector<tag_api_obj> tags_api::get_trending_tags(const std::string& after_tag, uint32_t limit) const
{
    try
    {
        return guard().with_read_lock([&]() { return _impl->get_trending_tags(after_tag, limit); });
    }
    FC_CAPTURE_AND_RETHROW((after_tag)(limit))
}

std::vector<std::pair<std::string, uint32_t>> tags_api::get_tags_used_by_author(const std::string& author) const
{
    try
    {
        return guard().with_read_lock([&]() { return _impl->get_tags_used_by_author(author); });
    }
    FC_CAPTURE_AND_RETHROW((author))
}

std::vector<std::pair<std::string, uint32_t>> tags_api::get_tags_by_category(const std::string& domain,
                                                                             const std::string& category) const
{
    try
    {
        return guard().with_read_lock([&]() { return _impl->get_tags_by_category(domain, category); });
    }
    FC_CAPTURE_AND_RETHROW((category))
}

std::vector<discussion> tags_api::get_discussions_by_trending(const discussion_query& query) const
{
    try
    {
        return guard().with_read_lock([&]() { return _impl->get_discussions_by_trending(query); });
    }
    FC_CAPTURE_AND_RETHROW((query))
}

std::vector<discussion> tags_api::get_discussions_by_created(const discussion_query& query) const
{
    try
    {
        return guard().with_read_lock([&]() { return _impl->get_discussions_by_created(query); });
    }
    FC_CAPTURE_AND_RETHROW((query))
}

std::vector<discussion> tags_api::get_discussions_by_hot(const discussion_query& query) const
{
    try
    {
        return guard().with_read_lock([&]() { return _impl->get_discussions_by_hot(query); });
    }
    FC_CAPTURE_AND_RETHROW((query))
}

discussion tags_api::get_content(const std::string& author, const std::string& permlink) const
{
    try
    {
        return guard().with_read_lock([&]() { return _impl->get_content(author, permlink); });
    }
    FC_CAPTURE_AND_RETHROW((author)(permlink))
}

std::vector<api::discussion> tags_api::get_contents(const std::vector<api::content_query>& queries) const
{
    try
    {
        return guard().with_read_lock([&]() { return _impl->get_contents(queries); });
    }
    FC_CAPTURE_AND_RETHROW((queries))
}

std::vector<discussion>
tags_api::get_comments(const std::string& parent_author, const std::string& parent_permlink, uint32_t depth) const
{
    try
    {
        return guard().with_read_lock([&]() { return _impl->get_comments(parent_author, parent_permlink, depth); });
    }
    FC_CAPTURE_AND_RETHROW((parent_author)(parent_permlink)(depth))
}

std::vector<discussion> tags_api::get_parents(const api::content_query& query) const
{
    try
    {
        return guard().with_read_lock([&]() { return _impl->get_parents(query); });
    }
    FC_CAPTURE_AND_RETHROW((query))
}

std::vector<discussion> tags_api::get_discussions_by_author(const api::discussion_query& query) const
{
    try
    {
        return guard().with_read_lock([&]() { return _impl->get_discussions_by_author(query); });
    }
    FC_CAPTURE_AND_RETHROW((query))
}

std::vector<api::discussion> tags_api::get_paid_posts_comments_by_author(const api::discussion_query& query) const
{
    try
    {
        return guard().with_read_lock([&]() { return _impl->get_paid_posts_comments_by_author(query); });
    }
    FC_CAPTURE_AND_RETHROW((query))
}

} // namespace tags
} // namespace scorum
