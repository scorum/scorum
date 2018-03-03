#include "actor.hpp"

namespace scorum {
namespace chain {
class database_trx_integration_fixture;
}
}

using scorum::chain::database_trx_integration_fixture;

class ActorActions
{
public:
    ActorActions(database_trx_integration_fixture& fix, const Actor& a);

    void create();
    void transfer_to_vest(const Actor& a, asset amount);
    void transfer(const Actor& a, asset amount);
    void give_sp(const Actor& a, int amount);
    void give_scr(const Actor& a, int amount);

    // legacy
    void give_power(const Actor& a, int amount)
    {
        return give_sp(a, amount);
    }

private:
    database_trx_integration_fixture& _f;
    const Actor& _actor;
};
