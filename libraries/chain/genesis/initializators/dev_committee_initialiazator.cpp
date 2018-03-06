#include <scorum/chain/genesis/initializators/dev_committee_initialiazator.hpp>

#include <scorum/chain/genesis/genesis_state.hpp>
#include <scorum/chain/services/development_committee.hpp>
#include <scorum/chain/services/account.hpp>

namespace scorum {
namespace chain {
namespace genesis {

void dev_committee_initializator_impl::on_apply(initializator_context& ctx)
{
    auto& committee_service = ctx.services().development_committee_service();
    auto& acount_service = ctx.services().account_service();

    for (const auto& member : ctx.genesis_state().development_committee)
    {
        acount_service.check_account_existence(member);
    }

    for (const auto& member : ctx.genesis_state().development_committee)
    {
        committee_service.add_member(member);
    }
}

} // namespace genesis
} // namespace scorum
} // namespace chain
