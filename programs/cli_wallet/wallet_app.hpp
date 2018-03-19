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
    virtual fc::variant process_command(const std::string& command, const fc::variants& args = fc::variants());

private:
    void register_types_completions();
    void register_commands_api_completions();
};
}
