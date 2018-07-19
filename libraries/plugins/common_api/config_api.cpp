#include <scorum/common_api/config_api.hpp>

#include <fc/exception/exception.hpp>

#include <regex>

namespace scorum {

const std::string config_api::defailt_api_name = "api";

config_api::configs_by_api_type config_api::_instances_by_api = config_api::configs_by_api_type();

config_api::config_api(const std::string& api_name)
    : _api_name(api_name)
    , max_blockchain_history_depth(_max_blockchain_history_depth)
    , max_blocks_history_depth(_max_blocks_history_depth)
    , max_budgets_list_size(_max_budgets_list_size)
    , max_discussions_list_size(_max_discussions_list_size)
    , lookup_limit(_lookup_limit)
    , tags_to_analize_count(_tags_to_analize_count)
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
    , max_blockchain_history_depth(_max_blockchain_history_depth)
    , max_blocks_history_depth(_max_blocks_history_depth)
    , max_budgets_list_size(_max_budgets_list_size)
    , max_discussions_list_size(_max_discussions_list_size)
    , lookup_limit(_lookup_limit)
    , tags_to_analize_count(_tags_to_analize_count)
{
}

#define STR(field) BOOST_PP_STRINGIZE(field)

void config_api::get_program_options(boost::program_options::options_description& cli,
                                     boost::program_options::options_description& cfg)
{
    namespace po = boost::program_options;
    // clang-format off
    cli.add_options()(get_option_name(STR(max_blockchain_history_depth)).c_str(), po::value<uint32_t>(),
                      get_option_description(STR(max_blockchain_history_depth)).c_str())
                     (get_option_name(STR(max_blocks_history_depth)).c_str(), po::value<uint32_t>(),
                      get_option_description(STR(max_blocks_history_depth)).c_str())
                     (get_option_name(STR(max_budgets_list_size)).c_str(), po::value<uint32_t>(),
                      get_option_description(STR(max_budgets_list_size)).c_str())
                     (get_option_name(STR(max_discussions_list_size)).c_str(), po::value<uint32_t>(),
                      get_option_description(STR(max_discussions_list_size)).c_str())
                     (get_option_name(STR(lookup_limit)).c_str(), po::value<uint32_t>(),
                      get_option_description(STR(lookup_limit)).c_str())
                     (get_option_name(STR(tags_to_analize_count)).c_str(), po::value<uint32_t>(),
                      get_option_description(STR(tags_to_analize_count)).c_str());
    // clang-format on
    cfg.add(cli);
}

void config_api::set_options(const boost::program_options::variables_map& options)
{
    config_api clean_config(api_name());

    if (options.count(get_option_name(STR(max_blockchain_history_depth))))
    {
        clean_config._max_blockchain_history_depth
            = options.at(get_option_name(STR(max_blockchain_history_depth))).as<uint32_t>();
    }
    else if (options.count(get_option_name(STR(max_blockchain_history_depth), true)))
    {
        clean_config._max_blockchain_history_depth
            = options.at(get_option_name(STR(max_blockchain_history_depth), true)).as<uint32_t>();
    }

    if (options.count(get_option_name(STR(max_blocks_history_depth))))
    {
        clean_config._max_blocks_history_depth
            = options.at(get_option_name(STR(max_blocks_history_depth))).as<uint32_t>();
    }
    else if (options.count(get_option_name(STR(max_blocks_history_depth), true)))
    {
        clean_config._max_blocks_history_depth
            = options.at(get_option_name(STR(max_blocks_history_depth), true)).as<uint32_t>();
    }

    if (options.count(get_option_name(STR(max_budgets_list_size))))
    {
        clean_config._max_budgets_list_size = options.at(get_option_name(STR(max_budgets_list_size))).as<uint32_t>();
    }
    else if (options.count(get_option_name(STR(max_budgets_list_size), true)))
    {
        clean_config._max_budgets_list_size
            = options.at(get_option_name(STR(max_budgets_list_size), true)).as<uint32_t>();
    }

    if (options.count(get_option_name(STR(max_discussions_list_size))))
    {
        clean_config._max_discussions_list_size
            = options.at(get_option_name(STR(max_discussions_list_size))).as<uint32_t>();
    }
    else if (options.count(get_option_name(STR(max_discussions_list_size), true)))
    {
        clean_config._max_discussions_list_size
            = options.at(get_option_name(STR(max_discussions_list_size), true)).as<uint32_t>();
    }

    if (options.count(get_option_name(STR(lookup_limit))))
    {
        clean_config._lookup_limit = options.at(get_option_name(STR(lookup_limit))).as<uint32_t>();
    }
    else if (options.count(get_option_name(STR(lookup_limit), true)))
    {
        clean_config._lookup_limit = options.at(get_option_name(STR(lookup_limit), true)).as<uint32_t>();
    }

    if (options.count(get_option_name(STR(tags_to_analize_count))))
    {
        clean_config._tags_to_analize_count = options.at(get_option_name(STR(tags_to_analize_count))).as<uint32_t>();
    }
    else if (options.count(get_option_name(STR(tags_to_analize_count), true)))
    {
        clean_config._tags_to_analize_count
            = options.at(get_option_name(STR(tags_to_analize_count), true)).as<uint32_t>();
    }

    // recreate to reset constants
    config_api::_instances_by_api[api_name()].reset(new config_api(clean_config));
}

std::string config_api::get_option_name(const char* field, bool default_api)
{
    static const auto regex = std::regex("_");
    std::string result;
    result = (default_api) ? defailt_api_name : api_name() + "-" + field;
    return std::regex_replace(result, regex, "-");
}

std::string config_api::get_option_description(const char* field, bool default_api)
{
    static const auto regex = std::regex("_");
    std::string result = std::regex_replace(field, regex, " ");
    result += " option for ";
    result += (default_api) ? defailt_api_name : api_name();
    return result;
}

config_api& get_api_config(std::string api_name)
{
    if (api_name.empty())
        api_name = config_api::defailt_api_name;

    auto config_it = config_api::_instances_by_api.find(api_name);
    if (config_api::_instances_by_api.end() == config_it)
    {
        config_it = config_api::_instances_by_api
                        .insert(std::make_pair(api_name, std::unique_ptr<config_api>(new config_api(api_name))))
                        .first;
    }
    return (*config_it->second);
}
}
