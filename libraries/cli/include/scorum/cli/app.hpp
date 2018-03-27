#pragma once

#include <scorum/cli/completionregistry.hpp>

#include <fc/variant.hpp>
#include <fc/thread/thread.hpp>

#include <string>
#include <functional>
#include <map>

namespace scorum {
namespace cli {
class app : public completion_registry
{
public:
    app();
    virtual ~app();

    virtual void start();
    virtual void stop();
    void wait();

    std::string get_secret(const std::string& prompt);
    void set_prompt(const std::string& prompt);
    void format_result(const std::string& command,
                       std::function<std::string(fc::variant, const fc::variants&)> formatter);

protected:
    virtual fc::variant process_command(const std::string& command, const fc::variants& args = fc::variants()) = 0;

private:
    void getline(const fc::string& prompt, fc::string& line);

    void run();

    std::string _prompt = ">>>";
    std::map<std::string, std::function<std::string(fc::variant, const fc::variants&)>> _result_formatters;
    fc::future<void> _run_complete;
};
}
}
