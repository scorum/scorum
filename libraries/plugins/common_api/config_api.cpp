#include <scorum/common_api/config_api.hpp>

namespace scorum {

config_api::configs_by_api_type config_api::_instances_by_api = config_api::configs_by_api_type();

config_api::config_api()
    : max_blockchain_history_depth(_max_blockchain_history_depth)
    , max_blocks_history_depth(_max_blocks_history_depth)
    , max_budgets_list_size(_max_budgets_list_size)
    , max_discussions_list_size(_max_discussions_list_size)
    , lookup_limit(_lookup_limit)
    , tags_to_analize_count(_tags_to_analize_count)
{
}

void config_api::set_options(const boost::program_options::variables_map& options)
{
    if (options.count(BOOST_PP_STRINGIZE(max_blockchain_history_depth)))
    {
        _max_blockchain_history_depth = options.at(BOOST_PP_STRINGIZE(max_blockchain_history_depth)).as<uint32_t>();
    }
    else if (options.count(BOOST_PP_STRINGIZE(max_blocks_history_depth)))
    {
        _max_blocks_history_depth = options.at(BOOST_PP_STRINGIZE(max_blocks_history_depth)).as<uint32_t>();
    }
    else if (options.count(BOOST_PP_STRINGIZE(max_budgets_list_size)))
    {
        _max_budgets_list_size = options.at(BOOST_PP_STRINGIZE(max_budgets_list_size)).as<uint32_t>();
    }
    else if (options.count(BOOST_PP_STRINGIZE(max_discussions_list_size)))
    {
        _max_discussions_list_size = options.at(BOOST_PP_STRINGIZE(max_discussions_list_size)).as<uint32_t>();
    }
    else if (options.count(BOOST_PP_STRINGIZE(lookup_limit)))
    {
        _lookup_limit = options.at(BOOST_PP_STRINGIZE(lookup_limit)).as<uint32_t>();
    }
    else if (options.count(BOOST_PP_STRINGIZE(tags_to_analize_count)))
    {
        _tags_to_analize_count = options.at(BOOST_PP_STRINGIZE(tags_to_analize_count)).as<uint32_t>();
    }
}

config_api& get_api_config(const std::string& api_name)
{
    auto config_it = config_api::_instances_by_api.find(api_name);
    if (config_api::_instances_by_api.end() == config_it)
    {
        config_it = config_api::_instances_by_api
                        .insert(std::make_pair(api_name, std::unique_ptr<config_api>(new config_api())))
                        .first;
    }
    return (*config_it->second);
}
}
