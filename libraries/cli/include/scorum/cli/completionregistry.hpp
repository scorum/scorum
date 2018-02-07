#pragma once

#include <map>
#include <regex>
#include <string>
#include <vector>

#include <fc/fixed_string.hpp>

namespace scorum {
namespace cli {

using hash_type = fc::fixed_string_24; // to pack ripemd160 hash

enum class completion_context_id : int
{
    empty = -1,
    commands = 0,
    custom_
};

struct completion_context
{
    completion_context(const std::string& line_regex_, const std::string& arg_regex_, bool _replace_match)
        : line_regex(line_regex_)
        , arg_regex(arg_regex_)
        , replace_match(_replace_match)
    {
    }

    completion_context(const std::string& arg_regex_, bool _replace_match)
        : arg_regex(arg_regex_)
        , replace_match(_replace_match)
    {
    }

    completion_context()
    {
    }

    std::string line_regex;
    std::string arg_regex;
    int id = (int)completion_context_id::commands;
    bool replace_match = false; // convert completions to input matching (look generate_completions)

    hash_type hash() const;
    bool empty() const
    {
        return line_regex.empty() && arg_regex.empty() && id == (int)completion_context_id::commands;
    }
};

class completion_registry
{
public:
    completion_registry();

    using completions_contexts_type = std::map<hash_type, completion_context>;
    using completions_incontext_type = std::vector<std::string>;
    using completions_incontext_store_type = std::pair<completions_incontext_type, completions_incontext_type>;
    using completions_type = std::map<int, completions_incontext_store_type>;

    const completions_incontext_type& get_completions(int context_id = (int)completion_context_id::commands) const;

    template <typename T, typename... Types> void registry_completions(const T& val, const Types&... vals)
    {
        _registry_ctx.add(val);
        registry_completions(vals...);
    }

    void registry_completions(const completion_context& ctx)
    {
        _registry_ctx.set_ctx(ctx);
        _registry_ctx.apply();
    }

    void registry_completions()
    {
        _registry_ctx.apply();
    }

    void registry_completions(const completions_incontext_type&, completion_context ctx = completion_context());

    int guess_completion_context(const char* line, const char* text, int start, int end);

private:
    bool generate_completions(int context_id, const std::string& match, const std::regex& r);

    class registry_ctx
    {
    public:
        explicit registry_ctx(completion_registry& p)
            : _this(p)
        {
        }
        void add(const std::string& next)
        {
            _completions.push_back(next);
        }
        void set_ctx(const completion_context& ctx)
        {
            _ctx = ctx;
        }
        void apply()
        {
            if (!_ctx.empty())
            {
                _ctx.id = next_context_id++;
            }
            _this.registry_completions(_completions, _ctx);
            _completions.clear();
            _ctx = completion_context();
        }

        int next_context_id = (int)completion_context_id::custom_;

    private:
        completion_registry& _this;
        completion_context _ctx;
        completion_registry::completions_incontext_type _completions;
    } _registry_ctx;
    completions_contexts_type _contexts;
    completions_type _completions;
};
}
}
