#include <scorum/common_api/config_api.hpp>

#include <fc/exception/exception.hpp>

#include <boost/algorithm/string.hpp>

namespace scorum {

const std::string config_api::default_api_name = "api";

config_api::configs_by_api_type config_api::_instances_by_api = config_api::configs_by_api_type();

config_api::config_api(const std::string& api_name)
    : _api_name(api_name)
    , max_blockchain_history_depth(_max_blockchain_history_depth)
    , max_blocks_history_depth(_max_blocks_history_depth)
    , max_budgets_list_size(_max_budgets_list_size)
    , max_discussions_list_size(_max_discussions_list_size)
    , lookup_limit(_lookup_limit)
    , tags_to_analize_count(_tags_to_analize_count)
    , max_timestamp_range_in_s(_max_timestamp_range_in_s)
{
}

config_api::config_api(config_api& config)
    : _api_name(config.api_name())
    , _max_blockchain_history_depth(config._max_blockchain_history_depth)
    , _max_blocks_history_depth(config._max_blocks_history_depth)
    , _max_budgets_list_size(config._max_budgets_list_size)
    , _max_discussions_list_size(config._max_discussions_list_size)
    , _lookup_limit(config._lookup_limit)
    , _tags_to_analize_count(config._tags_to_analize_count)
    , _max_timestamp_range_in_s(config._max_timestamp_range_in_s)
    , max_blockchain_history_depth(_max_blockchain_history_depth)
    , max_blocks_history_depth(_max_blocks_history_depth)
    , max_budgets_list_size(_max_budgets_list_size)
    , max_discussions_list_size(_max_discussions_list_size)
    , lookup_limit(_lookup_limit)
    , tags_to_analize_count(_tags_to_analize_count)
    , max_timestamp_range_in_s(_max_timestamp_range_in_s)
{
}

namespace bpo = boost::program_options;

#define STR(field) BOOST_PP_STRINGIZE(field)

bpo::options_description config_api::get_options_descriptions() const
{
    bpo::options_description result;
    bpo::options_description_easy_init options(&result);
    get_option_description<uint32_t>(options, STR(max_blockchain_history_depth));
    get_option_description<uint32_t>(options, STR(max_blocks_history_depth));
    get_option_description<uint32_t>(options, STR(max_budgets_list_size));
    get_option_description<uint32_t>(options, STR(max_discussions_list_size));
    get_option_description<uint32_t>(options, STR(lookup_limit));
    get_option_description<uint32_t>(options, STR(tags_to_analize_count));
    get_option_description<uint32_t>(options, STR(max_timestamp_range_in_s));
    return result;
}

void config_api::set_options(const bpo::variables_map& options)
{
    config_api clean_config(api_name());

    set_option<uint32_t>(options, clean_config._max_blockchain_history_depth, STR(max_blockchain_history_depth));
    set_option<uint32_t>(options, clean_config._max_blocks_history_depth, STR(max_blocks_history_depth));
    set_option<uint32_t>(options, clean_config._max_budgets_list_size, STR(max_budgets_list_size));
    set_option<uint32_t>(options, clean_config._max_discussions_list_size, STR(max_discussions_list_size));
    set_option<uint32_t>(options, clean_config._lookup_limit, STR(lookup_limit));
    set_option<uint32_t>(options, clean_config._tags_to_analize_count, STR(tags_to_analize_count));
    set_option<uint32_t>(options, clean_config._max_timestamp_range_in_s, STR(max_timestamp_range_in_s));

    // recreate to reset constants
    config_api::_instances_by_api[api_name()].reset(new config_api(clean_config));
}

template <typename MemberType>
void config_api::get_option_description(bpo::options_description_easy_init& options, const char* member_name) const
{
    options(get_option_name(member_name, api_name()).c_str(), bpo::value<MemberType>(),
            get_option_description(member_name).c_str());
}

template <typename MemberType>
void config_api::set_option(const bpo::variables_map& options, MemberType& member, const char* member_name)
{
    if (options.count(get_option_name(member_name, api_name())))
    {
        member = options.at(get_option_name(member_name, api_name())).as<MemberType>();
    }
    else if (options.count(get_option_name(member_name, default_api_name)))
    {
        member = options.at(get_option_name(member_name, default_api_name)).as<MemberType>();
    }
}

std::string config_api::get_option_name(const char* field, const std::string& api_name) const
{
    std::string result = api_name;
    result += "-";
    result += field;
    return boost::algorithm::replace_all_copy(result, "_", "-");
}

std::string config_api::get_option_description(const char* field) const
{
    std::string result = boost::algorithm::replace_all_copy(std::string(field), "_", " ");
    result += " option for ";
    result += api_name();
    return result;
}

config_api& get_api_config(std::string api_name)
{
    if (api_name.empty())
        api_name = config_api::default_api_name;

    auto config_it = config_api::_instances_by_api.find(api_name);
    if (config_api::_instances_by_api.end() == config_it)
    {
        config_it
            = config_api::_instances_by_api.emplace(api_name, std::unique_ptr<config_api>(new config_api(api_name)))
                  .first;
    }
    return (*config_it->second);
}
}
