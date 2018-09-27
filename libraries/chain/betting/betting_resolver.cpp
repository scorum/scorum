#include <scorum/chain/betting/betting_resolver.hpp>

#include <scorum/chain/betting/betting_service.hpp>

#include <scorum/chain/data_service_factory.hpp>
#include <scorum/chain/services/matched_bet.hpp>
#include <scorum/chain/services/bet.hpp>
#include <scorum/chain/services/account.hpp>

namespace scorum {
namespace chain {
betting_resolver::betting_resolver(betting_service_i& betting_svc,
                                   matched_bet_service_i& matched_bet_svc,
                                   bet_service_i& bet_svc,
                                   account_service_i& account_svc)
    : _betting_svc(betting_svc)
    , _matched_bet_svc(matched_bet_svc)
    , _bet_svc(bet_svc)
    , _account_svc(account_svc)
{
}

void betting_resolver::resolve_matched_bets(const game_id_type& game_id,
                                            const fc::shared_flat_set<wincase_type>& results) const
{
    auto matched_bets = _matched_bet_svc.get_bets(game_id);

    for (const matched_bet_object& matched_bet : matched_bets)
    {
        const auto& bet1 = _bet_svc.get_bet(matched_bet.bet1);
        const auto& bet2 = _bet_svc.get_bet(matched_bet.bet2);

        auto fst_won = results.find(bet1.wincase) != results.end();
        auto snd_won = results.find(bet2.wincase) != results.end();

        if (fst_won)
            _account_svc.increase_balance(bet1.better, matched_bet.matched_bet1_stake + matched_bet.matched_bet2_stake);
        else if (snd_won)
            _account_svc.increase_balance(bet2.better, matched_bet.matched_bet1_stake + matched_bet.matched_bet2_stake);
        else
        {
            _account_svc.increase_balance(bet1.better, matched_bet.matched_bet1_stake);
            _account_svc.increase_balance(bet2.better, matched_bet.matched_bet2_stake);
        }
    }

    _matched_bet_svc.remove_all(matched_bets);
}
}
}
