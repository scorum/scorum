#include "wallet_app.hpp"

#include <scorum/protocol/config.hpp>

namespace scorum {

wallet_app::wallet_app()
{
    register_types_completions();
}

void wallet_app::start()
{
    register_commands_api_completions();
    cli::app::start();
}

fc::variant wallet_app::process_command(const std::string& command, const fc::variants& args /*= fc::variants()*/)
{
    return receive_call(0, command, args);
}

void wallet_app::register_types_completions()
{
    std::string completion_mask(SCORUM_CURRENCY_PRECISION, '0');
    completions_incontext_type completions = { "0." + completion_mask + " SCR", "0." + completion_mask + " SP" };
    std::string completion_reg(R"(\d+\.)");
    for (int ci = 0; ci < SCORUM_CURRENCY_PRECISION + 1; ++ci)
    {
        registry_completions(completions, cli::completion_context(completion_reg, true));
        completion_reg.append(R"(\d)");
    }
    registry_completions("SCR\"", cli::completion_context(R"(.+"\d+\.\d+\s+SC)", R"(SC)", false));
    registry_completions("SP\"", cli::completion_context(R"(.+"\d+\.\d+\s+SP)", R"(SP)", false));
}
void wallet_app::register_commands_api_completions()
{
    completions_incontext_type commands = get_method_names(0);
    registry_completions(commands);
    registry_completions(commands, cli::completion_context(R"(gethelp\s+[a-z_]+)", R"([a-z_]+)", false));
}
}
