#include <scorum/common_api/config_api.hpp>

#include <fc/exception/exception.hpp>

#include <regex>

namespace scorum {

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

void config_api::get_program_options(boost::program_options::options_description& cli,
                                     boost::program_options::options_description& cfg)
{
    namespace po = boost::program_options;
    // clang-format off
    cli.add_options()(get_option_name(BOOST_PP_STRINGIZE(max_blockchain_history_depth)).c_str(), po::value<uint32_t>(),
                      get_option_description(BOOST_PP_STRINGIZE(max_blockchain_history_depth)).c_str())
                     (get_option_name(BOOST_PP_STRINGIZE(max_blocks_history_depth)).c_str(), po::value<uint32_t>(),
                      get_option_description(BOOST_PP_STRINGIZE(max_blocks_history_depth)).c_str())
                     (get_option_name(BOOST_PP_STRINGIZE(max_budgets_list_size)).c_str(), po::value<uint32_t>(),
                      get_option_description(BOOST_PP_STRINGIZE(max_budgets_list_size)).c_str())
                     (get_option_name(BOOST_PP_STRINGIZE(max_discussions_list_size)).c_str(), po::value<uint32_t>(),
                      get_option_description(BOOST_PP_STRINGIZE(max_discussions_list_size)).c_str())
                     (get_option_name(BOOST_PP_STRINGIZE(lookup_limit)).c_str(), po::value<uint32_t>(),
                      get_option_description(BOOST_PP_STRINGIZE(lookup_limit)).c_str())
                     (get_option_name(BOOST_PP_STRINGIZE(tags_to_analize_count)).c_str(), po::value<uint32_t>(),
                      get_option_description(BOOST_PP_STRINGIZE(tags_to_analize_count)).c_str());
    // clang-format on
    cfg.add(cli);
}

void config_api::set_options(const boost::program_options::variables_map& options)
{
    if (options.count(get_option_name(BOOST_PP_STRINGIZE(max_blockchain_history_depth))))
    {
        _max_blockchain_history_depth
            = options.at(get_option_name(BOOST_PP_STRINGIZE(max_blockchain_history_depth))).as<uint32_t>();
    }
    else if (options.count(get_option_name(BOOST_PP_STRINGIZE(max_blocks_history_depth))))
    {
        _max_blocks_history_depth
            = options.at(get_option_name(BOOST_PP_STRINGIZE(max_blocks_history_depth))).as<uint32_t>();
    }
    else if (options.count(get_option_name(BOOST_PP_STRINGIZE(max_budgets_list_size))))
    {
        _max_budgets_list_size = options.at(get_option_name(BOOST_PP_STRINGIZE(max_budgets_list_size))).as<uint32_t>();
    }
    else if (options.count(get_option_name(BOOST_PP_STRINGIZE(max_discussions_list_size))))
    {
        _max_discussions_list_size
            = options.at(get_option_name(BOOST_PP_STRINGIZE(max_discussions_list_size))).as<uint32_t>();
    }
    else if (options.count(get_option_name(BOOST_PP_STRINGIZE(lookup_limit))))
    {
        _lookup_limit = options.at(get_option_name(BOOST_PP_STRINGIZE(lookup_limit))).as<uint32_t>();
    }
    else if (options.count(get_option_name(BOOST_PP_STRINGIZE(tags_to_analize_count))))
    {
        _tags_to_analize_count = options.at(get_option_name(BOOST_PP_STRINGIZE(tags_to_analize_count))).as<uint32_t>();
    }
    // recreate itself to reset constants
    config_api::_instances_by_api[api_name()].reset(new config_api(*this));
}

std::string config_api::get_option_name(const char* field)
{
    static const auto regex = std::regex("_");
    std::string result;
    result = api_name() + "-" + field;
    return std::regex_replace(result, regex, "-");
}
std::string config_api::get_option_description(const char* field)
{
    static const auto regex = std::regex("_");
    return std::regex_replace(field, regex, " ") + " option for " + api_name();
}

config_api& get_api_config(const std::string& api_name)
{
    FC_ASSERT(!api_name.empty());
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
