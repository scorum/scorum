#pragma once

#include <memory>

#include <boost/program_options.hpp>
#include <map>

namespace scorum {

class config_api
{
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

    void set_options(const boost::program_options::variables_map& options);

protected:
    config_api();

private:
    friend config_api& get_api_config(const std::string& api_name = std::string());

    using configs_by_api_type = std::map<std::string, std::unique_ptr<config_api>>;
    static configs_by_api_type _instances_by_api;
};

config_api& get_api_config(const std::string& api_name);
}

#define MAX_BLOCKCHAIN_HISTORY_DEPTH auto(get_api_config().max_blockchain_history_depth)
#define MAX_BLOCKS_HISTORY_DEPTH auto(get_api_config().max_blocks_history_depth)
#define MAX_BUDGETS_LIST_SIZE auto(get_api_config().max_budgets_list_size)
#define MAX_DISCUSSIONS_LIST_SIZE auto(get_api_config().max_discussions_list_size)
#define LOOKUP_LIMIT auto(get_api_config().lookup_limit)
#define TAGS_TO_ANALIZE_COUNT auto(get_api_config().tags_to_analize_count)
