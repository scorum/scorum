#include <scorum/chain/betting/betting_resolver.hpp>

#include <scorum/chain/data_service_factory.hpp>
#include <scorum/chain/services/pending_bet.hpp>
#include <scorum/chain/services/matched_bet.hpp>
#include <scorum/chain/services/bet.hpp>
#include <scorum/chain/services/account.hpp>

namespace scorum {
namespace chain {
namespace betting {
betting_resolver::betting_resolver(pending_bet_service_i& pending_bet_svc,
                                   matched_bet_service_i& matched_bet_svc,
                                   bet_service_i& bet_svc,
                                   account_service_i& account_svc)
    : _pending_bet_svc(pending_bet_svc)
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
            resolve_balance(bet1.better, bet2.better, matched_bet.matched_bet1_stake, matched_bet.matched_bet2_stake);
        else if (snd_won)
            resolve_balance(bet2.better, bet1.better, matched_bet.matched_bet2_stake, matched_bet.matched_bet1_stake);
        else
        {
            return_balance(bet1.better, matched_bet.matched_bet1_stake);
            return_balance(bet2.better, matched_bet.matched_bet2_stake);
        }
    }
}

void betting_resolver::return_pending_bets(const game_id_type& game_id) const
{
    auto pending_bets = _pending_bet_svc.get_bets(game_id);

    for (const pending_bet_object& pending_bet : pending_bets)
    {
        const auto& bet = _bet_svc.get_bet(pending_bet.bet);
        return_balance(bet.better, bet.rest_stake);
    }
}

void betting_resolver::resolve_balance(const account_name_type& winner_name,
                                       const account_name_type& loser_name,
                                       const asset& winner_stake,
                                       const asset& loser_stake) const
{
    const auto& winner_acc = _account_svc.get_account(winner_name);
    const auto& loser_acc = _account_svc.get_account(loser_name);

    _account_svc.increase_balance(winner_acc, winner_stake + loser_stake);
    _account_svc.decrease_balance(loser_acc, loser_stake);
}

void betting_resolver::return_balance(const protocol::account_name_type& acc_name, const protocol::asset& stake) const
{
    const auto& acc = _account_svc.get_account(acc_name);

    _account_svc.increase_balance(acc, stake);
}
}
}
}
