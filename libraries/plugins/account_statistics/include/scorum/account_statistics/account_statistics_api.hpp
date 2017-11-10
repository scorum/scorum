#pragma once

#include <scorum/account_statistics/account_statistics_plugin.hpp>

#include <fc/api.hpp>

namespace scorum {
namespace app {
struct api_context;
}
}

namespace scorum {
namespace account_statistics {

namespace detail {
class account_statistics_api_impl;
}

class account_statistics_api
{
public:
    account_statistics_api(const scorum::app::api_context& ctx);

    void on_api_startup();

private:
    std::shared_ptr<detail::account_statistics_api_impl> _my;
};
}
} // scorum::account_statistics

FC_API(scorum::account_statistics::account_statistics_api, )