#pragma once
#include <scorum/protocol/betting/game.hpp>
#include <scorum/protocol/betting/market.hpp>
#include <scorum/protocol/betting/wincase.hpp>

namespace scorum {
namespace protocol {
namespace betting {
void validate_game(const game_type& game, const fc::flat_set<market_type>& markets);
void validate_wincases(const fc::flat_set<wincase_type>& wincases);
void validate_wincase(const wincase_type& wincase);
void validate_if_wincase_in_game(const game_type& game, const wincase_type& wincase);
void validate_bet_ids(const fc::flat_set<int64_t>& bet_ids);
}
}
}
