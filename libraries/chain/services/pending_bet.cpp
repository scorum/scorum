#include <scorum/chain/services/pending_bet.hpp>

namespace scorum {
namespace chain {

dbs_pending_bet::dbs_pending_bet(database& db)
    : base_service_type(db)
{
}

void dbs_pending_bet::foreach_pending_bets(dbs_pending_bet::pending_bet_call_type&& call)
{
    foreach_by<by_id>(call);
}
}
}
