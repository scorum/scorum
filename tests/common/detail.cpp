#include "detail.hpp"
#include <scorum/chain/schema/bet_objects.hpp>

#include <fc/io/json.hpp>

boost::uuids::uuid gen_uuid(const std::string& seed)
{
    boost::uuids::uuid uuid_ns = boost::uuids::string_generator()("e629f9aa-6b2c-46aa-8fa8-36770e7a7a5f");
    boost::uuids::name_generator uuid_gen(uuid_ns);

    return uuid_gen(seed);
}

bool compare_bet_data(const scorum::chain::bet_data& lhs, const scorum::chain::bet_data& rhs)
{
    // clang-format off
    return lhs.odds == rhs.odds
            && lhs.better == rhs.better
            && lhs.created == rhs.created
            && lhs.kind == rhs.kind
            && lhs.stake == rhs.stake
            && lhs.uuid == rhs.uuid
            && lhs.wincase == rhs.wincase;
    // clang-format on
}

namespace detail {

std::string flatten(const std::string& json)
{
    return fc::json::to_string(fc::json::from_string(json));
}
}
