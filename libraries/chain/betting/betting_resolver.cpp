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
            increase_balance(bet1.better, matched_bet.matched_bet1_stake + matched_bet.matched_bet2_stake);
        else if (snd_won)
            increase_balance(bet2.better, matched_bet.matched_bet1_stake + matched_bet.matched_bet2_stake);
        else
        {
            increase_balance(bet1.better, matched_bet.matched_bet1_stake);
            increase_balance(bet2.better, matched_bet.matched_bet2_stake);
        }
    }
}

void betting_resolver::return_pending_bets(const game_id_type& game_id, pending_bet_kind kind) const
{
    auto pending_bets = _pending_bet_svc.get_bets(game_id, kind);
    return_pending_bets(pending_bets);
}

void betting_resolver::return_pending_bets(const game_id_type& game_id) const
{
    auto pending_bets = _pending_bet_svc.get_bets(game_id);
    return_pending_bets(pending_bets);
}

void betting_resolver::return_matched_bets(const game_id_type& game_id) const
{
    auto matched_bets = _matched_bet_svc.get_bets(game_id);

    for (const matched_bet_object& matched_bet : matched_bets)
    {
        const auto& bet1 = _bet_svc.get_bet(matched_bet.bet1);
        const auto& bet2 = _bet_svc.get_bet(matched_bet.bet2);

        increase_balance(bet1.better, matched_bet.matched_bet1_stake);
        increase_balance(bet2.better, matched_bet.matched_bet2_stake);
    }
}

void betting_resolver::return_bets(const std::vector<std::reference_wrapper<const bet_object>>& bets) const
{
    for (const bet_object& bet : bets)
    {
        increase_balance(bet.better, bet.stake);
    }
}

void betting_resolver::return_pending_bets(
    const std::vector<std::reference_wrapper<const pending_bet_object>>& pending_bets) const
{
    for (const pending_bet_object& pending_bet : pending_bets)
    {
        const auto& bet = _bet_svc.get_bet(pending_bet.bet);
        increase_balance(bet.better, bet.rest_stake);
    }
}

void betting_resolver::increase_balance(const protocol::account_name_type& acc_name, const protocol::asset& stake) const
{
    const auto& acc = _account_svc.get_account(acc_name);

    _account_svc.increase_balance(acc, stake);
}
}
}
}
