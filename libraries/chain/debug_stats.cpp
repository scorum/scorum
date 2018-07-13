#include <scorum/chain/debug_stats.hpp>
#include <scorum/chain/services/account.hpp>
#include <sstream>

namespace scorum {
namespace chain {
accounts_stats::accounts_stats(std::vector<std::reference_wrapper<const account_object>> accounts)
    : accounts(std::move(accounts))
{
}

accounts_stats::operator std::string() const
{
    std::ostringstream stream;

    stream << "\nAccounts stats: \n";
    for (const account_object& acc : accounts)
    {
        stream << "\t" << acc.name << "; " << acc.scorumpower << "; " << acc.balance << "\n";
    }

    return stream.str();
}

comment_stats::comment_stats(const comment_object& comment, const account_object& account)
    : comment(comment)
    , account(account)
{
}

comment_stats::operator std::string() const
{
    std::ostringstream stream;

    stream << "Comment author: " << comment.author << "; permlink: " << comment.permlink << "; "
           << account.scorumpower.to_string();

    return stream.str();
}
}
}