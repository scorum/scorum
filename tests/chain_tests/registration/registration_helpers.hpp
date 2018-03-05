#pragma once

#include <scorum/protocol/types.hpp>
#include <scorum/protocol/asset.hpp>
#include <scorum/chain/genesis/genesis_state.hpp>

#include <functional>
#include <vector>

namespace registration_helpers {

struct fuzzer
{
    uint64_t get_sin_f(uint64_t pos, uint64_t max_pos, uint64_t max_result);
};

using schedule_input_type = scorum::chain::genesis_state_type::registration_schedule_item;
using schedule_inputs_type = std::vector<schedule_input_type>;
using scorum::protocol::asset;

struct predictor
{
    void
    initialize(const asset& registration_supply, const asset& registration_bonus, const schedule_inputs_type& schedule);

    const asset& get_registration_supply()
    {
        return _registration_supply;
    }
    const asset& get_registration_bonus()
    {
        return _registration_bonus;
    }
    const schedule_inputs_type& get_registration_schedule()
    {
        return _registration_schedule;
    }

    //
    uint64_t schedule_input_max_pos();

    asset schedule_input_total_bonus(const asset& maximum_bonus);

    int schedule_input_bonus_percent_by_pos(uint64_t pos);

    asset schedule_input_predicted_allocate_by_pos(uint64_t pos, asset maximum_bonus);

    asset schedule_input_predicted_limit(uint64_t pass_through_blocks);

    bool schedule_input_pos_reach_limit(uint64_t& pos,
                                        uint64_t max_pos,
                                        asset predicted_limit,
                                        asset maximum_bonus,
                                        std::function<asset(uint64_t pos)> allocate_cash);

    asset schedule_input_vest_all(std::function<asset()> allocate_cash);

private:
    asset _registration_bonus;
    asset _registration_supply;
    schedule_inputs_type _registration_schedule;
};
}
