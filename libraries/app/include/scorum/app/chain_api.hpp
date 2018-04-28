#pragma once

#define API_CHAIN "chain_api"

#include <memory>
#include <fc/api.hpp>
#include <scorum/app/schema/chain_api_objects.hpp>

namespace scorum {

namespace chain {
class database;
}

namespace app {

struct api_context;

enum class reward_fund_type
{
    content_reward_fund_scr = 0,
    content_reward_fund_sp,
};

/**
* @brief The chain_api class shows blockchain entrails.
*/
class chain_api : public std::enable_shared_from_this<chain_api>
{
public:
    chain_api(const api_context& a);

    void on_api_startup();

    chain_properties_api_obj get_chain_properties() const;

    scheduled_hardfork_api_obj get_next_scheduled_hardfork() const;

    // money properties
    reward_fund_api_obj get_reward_fund(reward_fund_type) const;

    chain_capital_api_obj get_chain_capital() const;

private:
    chain::database& _db;
};
}
}

FC_REFLECT_ENUM(scorum::app::reward_fund_type, (content_reward_fund_scr)(content_reward_fund_sp))

FC_API(scorum::app::chain_api, (get_chain_properties)(get_next_scheduled_hardfork)(get_reward_fund)(get_chain_capital))
