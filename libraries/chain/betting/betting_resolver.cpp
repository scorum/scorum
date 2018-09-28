#include <scorum/chain/betting/betting_resolver.hpp>

#include <scorum/chain/data_service_factory.hpp>
#include <scorum/chain/services/matched_bet.hpp>
#include <scorum/chain/services/account.hpp>

namespace scorum {
namespace chain {
betting_resolver::betting_resolver(matched_bet_service_i& matched_bet_svc, account_service_i& account_svc)
    : _matched_bet_svc(matched_bet_svc)
    , _account_svc(account_svc)
{
}

void betting_resolver::resolve_matched_bets(const game_id_type& game_id,
                                            const fc::shared_flat_set<wincase_type>& results) const
{
    auto matched_bets = _matched_bet_svc.get_bets(game_id);

    for (const matched_bet_object& bet : matched_bets)
    {
        auto fst_won = results.find(bet.bet1_data.wincase) != results.end();
        auto snd_won = results.find(bet.bet2_data.wincase) != results.end();

        if (fst_won)
            _account_svc.increase_balance(bet.bet1_data.better, bet.bet1_data.stake + bet.bet2_data.stake);
        else if (snd_won)
            _account_svc.increase_balance(bet.bet2_data.better, bet.bet1_data.stake + bet.bet2_data.stake);
        else
        {
            _account_svc.increase_balance(bet.bet1_data.better, bet.bet1_data.stake);
            _account_svc.increase_balance(bet.bet2_data.better, bet.bet2_data.stake);
        }
    }

    _matched_bet_svc.remove_all(matched_bets);
}
}
}
