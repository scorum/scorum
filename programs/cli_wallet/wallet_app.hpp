#pragma once

#include <scorum/cli/app.hpp>
#include <fc/rpc/api_connection.hpp>
#include <fc/api.hpp>
#include <fc/exception/exception.hpp>

namespace scorum {

class wallet_app : public cli::app, public fc::api_connection
{
public:
    wallet_app();

    virtual void start();

protected:
    // cli::app
    virtual fc::variant process_command(const std::string& command, const fc::variants& args = fc::variants());

    // fc::api_connection
    virtual fc::variant send_call(fc::api_id_type, std::string, fc::variants = fc::variants())
    {
        FC_ASSERT(false);
        return fc::variant();
    }
    virtual fc::variant send_callback(uint64_t, fc::variants = fc::variants())
    {
        FC_ASSERT(false);
        return fc::variant();
    }
    virtual void send_notice(uint64_t, fc::variants = fc::variants())
    {
        FC_ASSERT(false);
    }
};
}
