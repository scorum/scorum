#pragma once

#include <scorum/chain/util/asset.hpp>
#include <scorum/chain/scorum_objects.hpp>

#include <scorum/protocol/asset.hpp>
#include <scorum/protocol/config.hpp>
#include <scorum/protocol/types.hpp>

#include <fc/reflect/reflect.hpp>

#include <fc/uint128.hpp>

namespace scorum { namespace chain { namespace util {

using scorum::protocol::asset;
using scorum::protocol::price;
using scorum::protocol::share_type;

using fc::uint128_t;

struct comment_reward_context
{
   share_type rshares;
   uint16_t   reward_weight = 0;
   asset      max_sbd;
   uint128_t  total_reward_shares2;
   asset      total_reward_fund_scorum;
   price      current_scorum_price;
   curve_id   reward_curve = quadratic;
   uint128_t  content_constant = SCORUM_CONTENT_CONSTANT_HF0;
};

uint64_t get_rshare_reward( const comment_reward_context& ctx );

inline uint128_t get_content_constant_s()
{
   return SCORUM_CONTENT_CONSTANT_HF0; // looking good for posters
}

uint128_t evaluate_reward_curve( const uint128_t& rshares, const curve_id& curve = quadratic, const uint128_t& content_constant = SCORUM_CONTENT_CONSTANT_HF0 );

inline bool is_comment_payout_dust( const price& p, uint64_t scorum_payout )
{
   return to_sbd( p, asset( scorum_payout, SCORUM_SYMBOL ) ) < SCORUM_MIN_PAYOUT_SBD;
}

} } } // scorum::chain::util

FC_REFLECT( scorum::chain::util::comment_reward_context,
   (rshares)
   (reward_weight)
   (max_sbd)
   (total_reward_shares2)
   (total_reward_fund_scorum)
   (current_scorum_price)
   (reward_curve)
   (content_constant)
   )
