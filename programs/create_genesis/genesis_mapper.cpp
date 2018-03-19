#include "genesis_mapper.hpp"

#include <scorum/protocol/base.hpp>

namespace scorum {
namespace util {

struct get_name_visitor
{
    using result_type = std::string;

    std::string operator()(const account_type& item) const
    {
        return item.name;
    }

    std::string operator()(const steemit_bounty_account_type& item) const
    {
        return item.name;
    }
};

struct init_empty_item_visitor
{
    using result_type = genesis_account_info_item_type;

    genesis_account_info_item_type operator()(const account_type&) const
    {
        return account_type{};
    }

    genesis_account_info_item_type operator()(const steemit_bounty_account_type&) const
    {
        return steemit_bounty_account_type{};
    }
};

class update_visitor
{
public:
    update_visitor(genesis_account_info_item_type& item, asset& accounts_supply, asset& steemit_bounty_accounts_supply)
        : _item(item)
        , _accounts_supply(accounts_supply)
        , _steemit_bounty_accounts_supply(steemit_bounty_accounts_supply)
    {
    }

    using result_type = void;

    void operator()(const account_type& item) const
    {
        account_type& update_item = _item.get<account_type>();
        if (update_item.name.empty())
            update_item.name = item.name;
        if (update_item.public_key == public_key_type())
            update_item.public_key = item.public_key;
        else if (update_item.public_key != item.public_key)
            FC_ASSERT(false, "Multiple public_key for same '${n}': '${pubk}', '${oldpubk}'.",
                      ("n", item.name)("pubk", item.public_key)("oldpubk", update_item.public_key));
        if (update_item.scr_amount.amount == 0)
        {
            update_item.scr_amount = item.scr_amount;
            _accounts_supply += update_item.scr_amount;
        }
        else if (update_item.scr_amount != item.scr_amount)
            FC_ASSERT(false, "Multiple scr_amount for '${n}': '${src}', '${oldsrc}'.",
                      ("n", item.name)("src", item.scr_amount)("oldsrc", update_item.scr_amount));
    }

    void operator()(const steemit_bounty_account_type& item) const
    {
        steemit_bounty_account_type& update_item = _item.get<steemit_bounty_account_type>();
        if (update_item.name.empty())
            update_item.name = item.name;
        if (update_item.sp_amount.amount == 0)
        {
            update_item.sp_amount = item.sp_amount;
            _steemit_bounty_accounts_supply += update_item.sp_amount;
        }
        else if (update_item.sp_amount != item.sp_amount)
            FC_ASSERT(false, "Multiple sp_amount for '${n}': '${src}', '${oldsp}'.",
                      ("n", item.name)("src", item.sp_amount)("oldsp", update_item.sp_amount));
    }

private:
    genesis_account_info_item_type& _item;
    asset& _accounts_supply;
    asset& _steemit_bounty_accounts_supply;
};

class save_visitor
{
public:
    explicit save_visitor(genesis_state_type& genesis)
        : _genesis(genesis)
    {
    }

    using result_type = void;

    void operator()(const account_type& item) const
    {
        return _genesis.accounts.push_back(item);
    }

    void operator()(const steemit_bounty_account_type& item) const
    {
        return _genesis.steemit_bounty_accounts.push_back(item);
    }

private:
    genesis_state_type& _genesis;
};

//

genesis_mapper::genesis_mapper()
{
}

void genesis_mapper::reset(const genesis_state_type& genesis)
{
    _uniq_items.clear();
    _accounts_supply.amount = 0;
    _steemit_bounty_accounts_supply.amount = 0;

    for (const auto& item : genesis.accounts)
    {
        update(item);
    }

    for (const auto& item : genesis.steemit_bounty_accounts)
    {
        update(item);
    }
}

void genesis_mapper::update(const genesis_account_info_item_type& item)
{
    genesis_account_info_item_type& genesis_item = _uniq_items[item.visit(get_name_visitor())][item.which()];
    if (!genesis_item.which())
        genesis_item = item.visit(init_empty_item_visitor());

    item.visit(update_visitor(genesis_item, _accounts_supply, _steemit_bounty_accounts_supply));
}

void genesis_mapper::update(const std::string& name,
                            const public_key_type& pubk,
                            const asset& scr_amount,
                            const asset& sp_amount)
{
    // sanitizing
    scorum::protocol::validate_account_name(name);
    FC_ASSERT(scr_amount.symbol() == SCORUM_SYMBOL, "Invalid token symbol.");
    FC_ASSERT(sp_amount.symbol() == SP_SYMBOL, "Invalid token symbol.");

    update(account_type{ name, pubk, scr_amount });

    if (sp_amount.amount > 0)
    {
        update(steemit_bounty_account_type{ name, sp_amount });
    }
}

void genesis_mapper::save(genesis_state_type& genesis)
{
    genesis.accounts.clear();
    genesis.steemit_bounty_accounts.clear();

    for (auto&& item : _uniq_items)
    {
        for (auto&& genesis_item : item.second)
        {
            genesis_item.second.visit(save_visitor(genesis));
        }
    }

    genesis.accounts_supply = _accounts_supply;
    FC_ASSERT(genesis.steemit_bounty_accounts_supply == _steemit_bounty_accounts_supply,
              "Invalid actual steemit_bounty_accounts_supply. Received '${as}', but required '${rs}'",
              ("as", _steemit_bounty_accounts_supply)("rs", genesis.steemit_bounty_accounts_supply));

    calculate_and_set_supply_rest(genesis);
}

void genesis_mapper::calculate_and_set_supply_rest(genesis_state_type& genesis)
{
    using scorum::protocol::share_type;

    share_type actually_supply_amount = genesis.accounts_supply.amount;
    actually_supply_amount += genesis.steemit_bounty_accounts_supply.amount;
    actually_supply_amount += genesis.founders_supply.amount;
    actually_supply_amount += genesis.rewards_supply.amount;
    actually_supply_amount += genesis.registration_supply.amount;
    actually_supply_amount += genesis.development_sp_supply.amount;
    actually_supply_amount += genesis.development_scr_supply.amount;

    asset actually_supply(actually_supply_amount, SCORUM_SYMBOL);
    FC_ASSERT(actually_supply_amount <= genesis.total_supply.amount,
              "Insufficient total_supply in input genesis: '${as}' > '${ts}'",
              ("as", actually_supply)("ts", genesis.total_supply));

    genesis.development_sp_supply += asset(genesis.total_supply.amount - actually_supply.amount, SP_SYMBOL);
}
}
}
