#pragma once

#include <stdint.h>
#include <vector>

#include <scorum/protocol/types.hpp>

namespace fc {
class time_point_sec;
}

namespace scorum {
namespace chain {

namespace dba {
template <typename> class db_accessor;
}

struct database_virtual_operations_emmiter_i;

struct dynamic_global_property_object;
struct pending_bet_object;
struct matched_bet_object;
struct matched_stake_type;

using matching_fix_list = std::map<scorum::uuid_type, std::vector<scorum::uuid_type>>;

struct betting_matcher_i
{
    virtual ~betting_matcher_i();

    virtual std::vector<std::reference_wrapper<const pending_bet_object>> match(const pending_bet_object& bet1) = 0;
};

int64_t create_matched_bet(dba::db_accessor<matched_bet_object>& _matched_bet_dba,
                           const pending_bet_object& bet1,
                           const pending_bet_object& bet2,
                           const scorum::chain::matched_stake_type& matched,
                           fc::time_point_sec head_block_time);

struct matcher;

class betting_matcher : public betting_matcher_i
{
public:
    betting_matcher(database_virtual_operations_emmiter_i&,
                    dba::db_accessor<pending_bet_object>&,
                    dba::db_accessor<matched_bet_object>&,
                    dba::db_accessor<dynamic_global_property_object>&);

    ~betting_matcher() override;

    std::vector<std::reference_wrapper<const pending_bet_object>> match(const pending_bet_object& bet2) override;

private:
    dba::db_accessor<pending_bet_object>& _pending_bet_dba;
    dba::db_accessor<dynamic_global_property_object>& _dprop_dba;

    const matching_fix_list _bets_matching_fix;

    std::unique_ptr<matcher> _impl;
};
}
}
