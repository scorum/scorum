#pragma once

#include <memory>

#include <boost/program_options.hpp>
#include <map>
#include <type_traits>

namespace scorum {

class config_api
{
    const std::string _api_name;

    uint32_t _max_blockchain_history_depth = 100;
    uint32_t _max_blocks_history_depth = 100;
    uint32_t _max_budgets_list_size = 100;
    uint32_t _max_discussions_list_size = 100;
    uint32_t _lookup_limit = 1000;
    uint32_t _tags_to_analize_count = 8; // also includes category, domain, language

public:
    const uint32_t& max_blockchain_history_depth;
    const uint32_t& max_blocks_history_depth;
    const uint32_t& max_budgets_list_size;
    const uint32_t& max_discussions_list_size;
    const uint32_t& lookup_limit;
    const uint32_t& tags_to_analize_count;

    void get_program_options(boost::program_options::options_description& cli,
                             boost::program_options::options_description& cfg);
    void set_options(const boost::program_options::variables_map& options);

    const std::string& api_name() const
    {
        return _api_name;
    }

protected:
    config_api() = delete;
    config_api(const std::string& api_name);
    config_api(config_api&);

private:
    friend config_api& get_api_config(const std::string& api_name = std::string("api"));

    std::string get_option_name(const char* field);
    std::string get_option_description(const char* field);

    using configs_by_api_type = std::map<std::string, std::unique_ptr<config_api>>;
    static configs_by_api_type _instances_by_api;
};

config_api& get_api_config(const std::string& api_name);
}

#define MAX_BLOCKCHAIN_HISTORY_DEPTH (get_api_config().max_blockchain_history_depth)
#define MAX_BLOCKS_HISTORY_DEPTH (get_api_config().max_blocks_history_depth)
#define MAX_BUDGETS_LIST_SIZE (get_api_config().max_budgets_list_size)
#define MAX_DISCUSSIONS_LIST_SIZE (get_api_config().max_discussions_list_size)
#define LOOKUP_LIMIT (get_api_config().lookup_limit)
#define TAGS_TO_ANALIZE_COUNT (get_api_config().tags_to_analize_count)
