#include "wallet_app.hpp"

namespace scorum {

wallet_app::wallet_app()
{
    registry_completions("0.000000000 SCR", "0.000000000 SP", cli::completion_context(R"(\d+\.)", true));
    registry_completions("SCR\"", cli::completion_context(R"(.+"\d+\.\d+\s+SC)", R"(SC)", false));
    registry_completions("SP\"", cli::completion_context(R"(.+"\d+\.\d+\s+SP)", R"(SP)", false));
}

void wallet_app::start()
{
    completions_incontext_type commands = get_method_names(0);
    registry_completions(commands);
    registry_completions(commands, cli::completion_context(R"(gethelp\s+[a-z_]+)", R"([a-z_]+)", false));
    parent_type::start();
}

fc::variant wallet_app::process_command(const std::string& command, const fc::variants& args /*= fc::variants()*/)
{
    return receive_call(0, command, args);
}
}
