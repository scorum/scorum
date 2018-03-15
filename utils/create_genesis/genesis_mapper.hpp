#pragma once

#include <map>
#include <string>

#include <scorum/chain/genesis/genesis_state.hpp>
#include <fc/static_variant.hpp>

namespace scorum {
namespace util {

using scorum::chain::genesis_state_type;
using account_type = genesis_state_type::account_type;
using steemit_bounty_account_type = genesis_state_type::steemit_bounty_account_type;

using scorum::protocol::asset;
using scorum::protocol::public_key_type;

using genesis_account_info_item_type = fc::static_variant<account_type, steemit_bounty_account_type>;

class genesis_mapper
{
public:
    genesis_mapper();

    void reset(const genesis_state_type&);

    void update(const genesis_account_info_item_type&);

    void update(const std::string& name,
                const std::string& recover_account,
                const public_key_type&,
                const asset& scr_amount,
                const asset& sp_amount);

    void save(genesis_state_type&);

private:
    void calculate_and_set_supply_rest(genesis_state_type& genesis);

    using genesis_account_info_item_map_by_type = std::map<int, genesis_account_info_item_type>;
    using genesis_account_info_items_type = std::map<std::string, genesis_account_info_item_map_by_type>;
    genesis_account_info_items_type _uniq_items;
    asset _accounts_supply = asset(0, SCORUM_SYMBOL);
    asset _steemit_bounty_accounts_supply = asset(0, SP_SYMBOL);
};
}
}
