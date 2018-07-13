#pragma once
#include <vector>
#include <string>
#include <scorum/chain/schema/account_objects.hpp>
#include <scorum/chain/schema/comment_objects.hpp>

namespace scorum {
namespace chain {

/*
 * Struct is used for debug logging purpose
 */
struct accounts_stats
{
    accounts_stats(std::vector<std::reference_wrapper<const account_object>> accounts);

    operator std::string() const;

    std::vector<std::reference_wrapper<const account_object>> accounts;
};

/*
 * Struct is used for debug logging purpose
 */
struct comment_stats
{
    comment_stats(const comment_object& comment, const account_object& account);

    operator std::string() const;

    const comment_object& comment;
    const account_object& account;
};
}
}