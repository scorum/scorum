#include <scorum/chain/betting/betting_matcher.hpp>

#include <scorum/chain/database/database_virtual_operations.hpp>
#include <scorum/chain/schema/bet_objects.hpp>
#include <scorum/chain/schema/dynamic_global_property_object.hpp>
#include <scorum/chain/betting/betting_math.hpp>
#include <scorum/chain/dba/db_accessor.hpp>
#include <scorum/protocol/betting/market.hpp>

namespace scorum {
namespace chain {

static const std::string matching_fix = R"([
    [
        "84911b2d-91d9-4375-80c3-8ccbc078e992",
        [
            "87e01c87-facd-4c68-93da-4e2dd4e1f546",
            "26e1f127-5d27-4c40-b551-f36e8e2a1ad2",
            "be0a2d8c-c525-42a2-a537-a7eab03aa319"
        ]
    ],
    [
        "c2dd413d-59a5-4bc8-b9f8-4efefb848cc7",
        [
            "c4193c3e-3ad9-4aa9-a2d5-df2637b23d32",
            "2da620fc-40bf-41b3-950b-89d509adf7cf"
        ]
    ],
    [
        "8256c36f-7399-4c01-967a-888f3730ec02",
        [
            "d0934027-e040-4f25-9202-c3e45b00bdd2"
        ]
    ],
    [
        "b4d3a62a-3ed2-48ee-8880-b23e6af9954b",
        [
            "bddff0ce-2e45-4f75-a9a6-dfbca27acd61"
        ]
    ],
    [
        "f45e1c57-64f4-4d18-a6f4-898df5a06b74",
        [
            "d26688cc-4d7b-49fa-801a-b6a3173ab0d3"
        ]
    ],
    [
        "11d1cd20-5809-481a-b07e-2017ecc5e5f8",
        [
            "d26688cc-4d7b-49fa-801a-b6a3173ab0d3"
        ]
    ],
    [
        "ba75d8ec-f800-414d-a9e1-8d90a6f443b7",
        [
            "d458ba6c-61b4-4cca-889d-8f10398bf232",
            "d26688cc-4d7b-49fa-801a-b6a3173ab0d3"
        ]
    ],
    [
        "bcda6311-9b6f-43ab-b834-ff04d90f31ea",
        [
            "10dc5d25-d690-4429-971f-8a7c1d0eec8f"
        ]
    ],
    [
        "7fc9e455-dd36-46b9-8534-8b230fa5c8b9",
        [
            "10dc5d25-d690-4429-971f-8a7c1d0eec8f"
        ]
    ],
    [
        "de35cb6b-fa81-4f4a-a0ae-86566162b744",
        [
            "ffb58868-1a17-471d-bf2c-e22c56111480",
            "605312d9-3c72-4552-8134-8e25974cbccd"
        ]
    ],
    [
        "d522f55f-db34-4be9-920d-4b82b8b7c635",
        [
            "10121a46-8178-478b-a80d-e265dc183560",
            "54cc0f28-0bc1-429b-ac98-3c543b64e3cd"
        ]
    ],
    [
        "a1a1f581-c614-440f-a599-b6f0cc3f997d",
        [
            "cd6ea2db-6a39-449f-9d1d-ab994cdc491d",
            "3eedea83-ccef-4797-8519-18bbd9c893fd"
        ]
    ],
    [
        "29fba9fc-edf7-4a3e-ba97-4429196e7e6c",
        [
            "c549c186-b3b2-4280-9e4b-4d5d029830ac",
            "f1990e32-0bfa-46c9-9bb6-ef14c3ce6bf3"
        ]
    ],
    [
        "dcfe2858-9599-4606-840b-25219d87816d",
        [
            "491f5618-14eb-4d61-91e6-090b247e0be0",
            "e15738c5-036c-4376-881d-6f35ce8fee50"
        ]
    ],
    [
        "a58fc953-7e7e-4d54-a0c1-a2a21f7d2f96",
        [
            "8a938b00-819a-45b9-a3e3-913f5b12b42c",
            "299396a3-6f29-4cb5-b13f-09ad4bbce295"
        ]
    ],
    [
        "8765a564-daaf-4971-b265-f43e6188b7aa",
        [
            "24494b24-407d-4c40-a742-9d2fadab55a5",
            "9bbc2474-bbf1-4a3a-93e2-28e484d6899e"
        ]
    ],
    [
        "72437504-9062-4730-974b-1ebcc66fd4e3",
        [
            "b660383c-9fd3-4f03-a1ac-c80ebb6e0826",
            "d6a616f5-803b-4c4a-8fb4-27873f95a2c2"
        ]
    ],
    [
        "5aed38ca-8a81-40de-890e-863cf27c0aee",
        [
            "c55e1f47-1e6d-44ec-9b37-12df4f6605ea",
            "2d88eb1c-bf49-482f-a3fa-11d324b24c27"
        ]
    ],
    [
        "a34d64cd-6a7f-4447-89d3-46dd8b60f012",
        [
            "76031c54-0856-4af5-8680-169ef19c4845",
            "e381cdff-7e4f-4aaf-b2a7-606b5eac2718"
        ]
    ],
    [
        "c7540a6f-4dd9-4d1c-8693-e009116c55e7",
        [
            "0460535a-7284-4dee-802b-1e3e634ee688"
        ]
    ],
    [
        "ef23721e-1134-4d07-be02-1cd3b6880f83",
        [
            "0460535a-7284-4dee-802b-1e3e634ee688"
        ]
    ],
    [
        "10976313-8081-4023-818f-30d9e6ffedc9",
        [
            "0460535a-7284-4dee-802b-1e3e634ee688"
        ]
    ],
    [
        "fe30cb81-0e8f-4ed5-b260-38c9e44a4725",
        [
            "0460535a-7284-4dee-802b-1e3e634ee688"
        ]
    ],
    [
        "9effbf85-ab41-468a-8c4a-991ea23a8a50",
        [
            "0460535a-7284-4dee-802b-1e3e634ee688"
        ]
    ],
    [
        "3568f452-47d9-456c-b085-c8ba83603016",
        [
            "d249b968-9359-42d8-a4aa-13af57bd6bae",
            "25a10696-58f2-4884-96d0-19ac235c04c8"
        ]
    ],
    [
        "bb1374e9-d014-4245-b2cd-e1ee0a0ca36b",
        [
            "0460535a-7284-4dee-802b-1e3e634ee688"
        ]
    ],
    [
        "d8ca2fcc-f6d0-4a06-b8d8-257424878b69",
        [
            "0460535a-7284-4dee-802b-1e3e634ee688"
        ]
    ],
    [
        "4de53ab4-eb14-4d5b-a02b-774c8d93c209",
        [
            "0460535a-7284-4dee-802b-1e3e634ee688"
        ]
    ],
    [
        "f59e6dc1-1d41-451e-8239-f9aaee629111",
        [
            "5c29e81d-4fd9-4f20-ba3b-03ab01d3c70f"
        ]
    ]
])";

// clang-format on

struct matcher
{
    matcher(database_virtual_operations_emmiter_i& emitter,
            dba::db_accessor<pending_bet_object>& pending_bet_dba,
            dba::db_accessor<matched_bet_object>& matched_bet_dba,
            dba::db_accessor<dynamic_global_property_object>& dprop_dba)
        : _virt_op_emitter(emitter)
        , _pending_bet_dba(pending_bet_dba)
        , _matched_bet_dba(matched_bet_dba)
        , _dprop_dba(dprop_dba)
    {
    }

    ~matcher() = default;

    bool can_be_matched(const pending_bet_object& bet)
    {
        return bet.data.stake * bet.data.odds > bet.data.stake;
    }

    bool is_bets_matched(const pending_bet_object& bet1, const pending_bet_object& bet2) const
    {
        return bet1.data.odds.inverted() == bet2.data.odds;
    }

    const pending_bet_object& get_bet(std::reference_wrapper<const pending_bet_object> ref)
    {
        return ref.get();
    }

    const pending_bet_object& get_bet(const pending_bet_object& bet)
    {
        return bet;
    }

    template <typename T>
    std::vector<std::reference_wrapper<const pending_bet_object>> match(const pending_bet_object& bet2, T& pending_bets)
    {
        std::vector<std::reference_wrapper<const pending_bet_object>> bets_to_cancel;

        for (const auto& bet_ref : pending_bets)
        {
            const pending_bet_object& bet1 = get_bet(bet_ref);

            if (!is_bets_matched(bet1, bet2))
                continue;

            auto matched = calculate_matched_stake(bet1.data.stake, bet2.data.stake, bet1.data.odds, bet2.data.odds);

            if (matched.bet1_matched.amount > 0 && matched.bet2_matched.amount > 0)
            {
                const auto matched_bet_id
                    = create_matched_bet(_matched_bet_dba, bet1, bet2, matched, _dprop_dba.get().time);

                auto bet1_old_stake = bet1.data.stake;
                auto bet2_old_stake = bet2.data.stake;

                _pending_bet_dba.update(bet1, [&](pending_bet_object& o) { o.data.stake -= matched.bet1_matched; });
                _pending_bet_dba.update(bet2, [&](pending_bet_object& o) { o.data.stake -= matched.bet2_matched; });

                _dprop_dba.update([&](dynamic_global_property_object& obj) {
                    obj.betting_stats.matched_bets_volume += matched.bet1_matched + matched.bet2_matched;
                    obj.betting_stats.pending_bets_volume -= matched.bet1_matched + matched.bet2_matched;
                });

                _virt_op_emitter.push_virtual_operation(protocol::bet_updated_operation{
                    bet1.game_uuid, bet1.data.better, bet1.data.uuid, bet1_old_stake, bet1.data.stake });
                _virt_op_emitter.push_virtual_operation(protocol::bet_updated_operation{
                    bet2.game_uuid, bet2.data.better, bet2.data.uuid, bet2_old_stake, bet2.data.stake });
                _virt_op_emitter.push_virtual_operation(protocol::bets_matched_operation(
                    bet1.data.better, bet2.data.better, bet1.get_uuid(), bet2.get_uuid(), matched.bet1_matched,
                    matched.bet2_matched, matched_bet_id));
            }

            if (!can_be_matched(bet1))
            {
                bets_to_cancel.emplace_back(bet1);
            }

            if (!can_be_matched(bet2))
            {
                bets_to_cancel.emplace_back(bet2);
                break;
            }
        }

        return bets_to_cancel;
    }

private:
    database_virtual_operations_emmiter_i& _virt_op_emitter;

    dba::db_accessor<pending_bet_object>& _pending_bet_dba;
    dba::db_accessor<matched_bet_object>& _matched_bet_dba;
    dba::db_accessor<dynamic_global_property_object>& _dprop_dba;
};

betting_matcher_i::~betting_matcher_i() = default;
betting_matcher::~betting_matcher() = default;

betting_matcher::betting_matcher(database_virtual_operations_emmiter_i& virt_op_emitter,
                                 dba::db_accessor<pending_bet_object>& pending_bet_dba,
                                 dba::db_accessor<matched_bet_object>& matched_bet_dba,
                                 dba::db_accessor<dynamic_global_property_object>& dprop_dba)
    : _pending_bet_dba(pending_bet_dba)
    , _dprop_dba(dprop_dba)
    , _bets_matching_fix(fc::json::from_string(matching_fix).as<matching_fix_list>())
    , _impl(std::make_unique<matcher>(virt_op_emitter, pending_bet_dba, matched_bet_dba, dprop_dba))
{
    dlog("${0}", ("0", fc::json::to_string(_bets_matching_fix)));
}

std::vector<std::reference_wrapper<const pending_bet_object>> betting_matcher::match(const pending_bet_object& bet2)
{
    try
    {
        auto key = std::make_tuple(bet2.game_uuid, create_opposite(bet2.get_wincase()));

        if (_bets_matching_fix.find(bet2.data.uuid) != _bets_matching_fix.end())
        {
            dlog("fix matching in block ${0}", ("0", _dprop_dba.get().head_block_number));
            dlog("fix matching for bet ${0}", ("0", bet2.data.uuid));

            std::vector<std::reference_wrapper<const pending_bet_object>> bets;

            for (auto uuid : _bets_matching_fix.at(bet2.data.uuid))
            {
                bets.push_back(_pending_bet_dba.get_by<by_uuid>(uuid));
                dlog("+ ${0}", ("0", uuid));
            }

            return _impl->match(bet2, bets);
        }
        else
        {
            auto bets = _pending_bet_dba.get_range_by<by_game_uuid_wincase_asc>(key);
            return _impl->match(bet2, bets);
        }
    }
    FC_CAPTURE_LOG_AND_RETHROW((bet2))
}

int64_t create_matched_bet(dba::db_accessor<matched_bet_object>& matched_bet_dba,
                           const pending_bet_object& bet1,
                           const pending_bet_object& bet2,
                           const matched_stake_type& matched,
                           fc::time_point_sec head_block_time)
{
    FC_ASSERT(bet1.game_uuid == bet2.game_uuid, "bets game id is not equal.");
    FC_ASSERT(bet1.get_wincase() == create_opposite(bet2.get_wincase()));

    return matched_bet_dba
        .create([&](matched_bet_object& obj) {
            obj.bet1_data = bet1.data;
            obj.bet2_data = bet2.data;
            obj.bet1_data.stake = matched.bet1_matched;
            obj.bet2_data.stake = matched.bet2_matched;
            obj.market = bet1.market;
            obj.game_uuid = bet1.game_uuid;
            obj.created = head_block_time;
        })
        .id._id;
}

} // namespace chain
} // namespace scorum
