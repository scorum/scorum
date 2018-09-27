#include <scorum/chain/betting/betting_resolver.hpp>

#include <scorum/chain/betting/betting_service.hpp>

#include <scorum/chain/data_service_factory.hpp>
#include <scorum/chain/services/matched_bet.hpp>
#include <scorum/chain/services/account.hpp>

namespace scorum {
namespace chain {
betting_resolver::betting_resolver(betting_service_i& betting_svc,
                                   matched_bet_service_i& matched_bet_svc,
                                   account_service_i& account_svc)
    : _betting_svc(betting_svc)
    , _matched_bet_svc(matched_bet_svc)
    , _account_svc(account_svc)
{
}

void betting_resolver::resolve_matched_bets(const game_id_type& game_id,
                                            const fc::shared_flat_set<wincase_type>& results) const
{
    auto matched_bets = _matched_bet_svc.get_bets(game_id);

    for (const matched_bet_object& bet : matched_bets)
    {
        auto fst_won = results.find(bet.wincase1) != results.end();
        auto snd_won = results.find(bet.wincase2) != results.end();

        if (fst_won)
            _account_svc.increase_balance(bet.better1, bet.stake1 + bet.stake2);
        else if (snd_won)
            _account_svc.increase_balance(bet.better2, bet.stake1 + bet.stake2);
        else
        {
            _account_svc.increase_balance(bet.better1, bet.stake1);
            _account_svc.increase_balance(bet.better2, bet.stake2);
        }
    }

    _matched_bet_svc.remove_all(matched_bets);
}
}
}
