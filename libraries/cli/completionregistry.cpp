#include <scorum/cli/completionregistry.hpp>

#include <fc/crypto/ripemd160.hpp>

#include <fc/exception/exception.hpp>

#include <sstream>

namespace scorum {
namespace cli {

hash_type completion_context::hash() const
{
    std::stringstream store;
    store << " " << line_regex << arg_regex;
    return fc::ripemd160().hash(store.str()).str();
}

completion_registry::completion_registry()
    : _registry_ctx(*this)
{
}

const completion_registry::completions_incontext_type&
completion_registry::get_completions(int context_id /*= (int)completion_context_id::commands */) const
{
    FC_ASSERT((int)completion_context_id::empty != context_id);
    completions_type::const_iterator it = _completions.find(context_id);
    FC_ASSERT(it != _completions.end());

    const completions_incontext_store_type& ret = it->second;
    if (!ret.second.empty())
        return ret.second;
    else
        return ret.first;
}

void completion_registry::registry_completions(const completions_incontext_type& completions,
                                               completion_context ctx /*= completion_context()*/)
{
    FC_ASSERT(!completions.empty());
    auto ctx_hash = ctx.hash();
    auto ctx_id = ctx.id;
    if ((int)completion_context_id::commands == ctx_id && !ctx.empty())
    {
        ctx.id = _registry_ctx.next_context_id++;
        ctx_id = ctx.id;
    }
    completions_contexts_type::const_iterator it = _contexts.find(ctx_hash);
    if (it == _contexts.end())
    {
        _contexts[ctx_hash] = ctx;
    }
    else
    {
        ctx_id = it->second.id;
    }
    _completions[ctx_id] = completions_incontext_store_type(completions, completions_incontext_type());
}

int completion_registry::guess_completion_context(const char* line, const char* text, int start, int end)
{
    if (!start)
    {
        return (int)completion_context_id::commands;
    }

    int context_id = (int)completion_context_id::empty;
    for (auto& ctx_item : _contexts)
    {
        const completion_context& ctx = ctx_item.second;
        if ((int)completion_context_id::commands == ctx.id)
            continue;

        if (!ctx.line_regex.empty())
        {
            std::regex reg(ctx.line_regex);
            if (!std::regex_match(line, reg))
            {
                continue;
            }
        }
        if (!ctx.arg_regex.empty())
        {
            std::regex reg(ctx.arg_regex);
            std::cmatch m;
            if (!std::regex_match(text, m, reg))
            {
                continue;
            }
            else
            {
                if (!ctx.replace_match || generate_completions(ctx.id, m.str(0), reg))
                {
                    context_id = ctx.id;
                    break;
                }
            }
        }
    }

    return context_id;
}

bool completion_registry::generate_completions(int context_id, const std::string& match, const std::regex& r)
{
    completions_type::iterator it = _completions.find(context_id);
    FC_ASSERT(it != _completions.end());

    completions_incontext_store_type& ret = it->second;
    const completions_incontext_type& in = ret.first;
    completions_incontext_type& out = ret.second;
    ;
    for (const std::string& item : in)
    {
        std::cmatch m;
        if (std::regex_search(item.c_str(), m, r))
        {
            std::string ritem(item);
            std::string rmatch(m.str(0));
            ritem.replace(0, rmatch.size(), match);
            out.push_back(ritem);
        }
    }
    return !out.empty();
}
}
}
