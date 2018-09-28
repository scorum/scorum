#pragma once
#include <scorum/app/plugin.hpp>
#include <scorum/chain/database/database.hpp>

namespace scorum {
namespace account_by_key {

#define ACCOUNT_BY_KEY_PLUGIN_NAME "account_by_key"

namespace detail {
class account_by_key_plugin_impl;
}

/**
 * @brief This plugin tracks accounts and its keys
 *
 * @ingroup plugins
 * @addtogroup account_by_key_plugin Account by key plugin
 */
class account_by_key_plugin : public scorum::app::plugin
{
public:
    account_by_key_plugin(scorum::app::application* app);
    ~account_by_key_plugin();

    std::string plugin_name() const override
    {
        return ACCOUNT_BY_KEY_PLUGIN_NAME;
    }
    virtual void plugin_set_program_options(boost::program_options::options_description& cli,
                                            boost::program_options::options_description& cfg) override;
    virtual void plugin_initialize(const boost::program_options::variables_map& options) override;
    virtual void plugin_startup() override;

    friend class detail::account_by_key_plugin_impl;
    std::unique_ptr<detail::account_by_key_plugin_impl> my;
};
}
} // scorum::account_by_key
