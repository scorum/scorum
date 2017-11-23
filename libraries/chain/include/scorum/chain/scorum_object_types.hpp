#pragma once
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/mem_fun.hpp>

//#include <graphene/db2/database.hpp>
#include <chainbase/chainbase.hpp>

#include <scorum/protocol/types.hpp>
#include <scorum/protocol/authority.hpp>

namespace scorum {
namespace chain {

namespace bip = chainbase::bip;
using namespace boost::multi_index;

using boost::multi_index_container;

using chainbase::object;
using chainbase::oid;
using chainbase::allocator;

using scorum::protocol::block_id_type;
using scorum::protocol::transaction_id_type;
using scorum::protocol::chain_id_type;
using scorum::protocol::account_name_type;
using scorum::protocol::share_type;

typedef bip::basic_string<char, std::char_traits<char>, allocator<char>> shared_string;
inline std::string to_string(const shared_string& str)
{
    return std::string(str.begin(), str.end());
}
inline void from_string(shared_string& out, const string& in)
{
    out.assign(in.begin(), in.end());
}

typedef bip::vector<char, allocator<char>> buffer_type;

struct by_id;

enum object_type
{
    dynamic_global_property_object_type,
    chain_property_object_type,
    account_object_type,
    account_authority_object_type,
    witness_object_type,
    transaction_object_type,
    block_summary_object_type,
    witness_schedule_object_type,
    comment_object_type,
    comment_vote_object_type,
    witness_vote_object_type,
    operation_object_type,
    account_history_object_type,
    hardfork_property_object_type,
    withdraw_vesting_route_object_type,
    owner_authority_history_object_type,
    account_recovery_request_object_type,
    change_recovery_account_request_object_type,
    escrow_object_type,
    decline_voting_rights_request_object_type,
    block_stats_object_type,
    reward_fund_object_type,
    vesting_delegation_object_type,
    vesting_delegation_expiration_object_type,
    budget_object_type,
    budget_with_schedule_object_type,
    budget_schedule_object_type
};

class dynamic_global_property_object;
class chain_property_object;
class account_object;
class account_authority_object;
class witness_object;
class transaction_object;
class block_summary_object;
class witness_schedule_object;
class comment_object;
class comment_vote_object;
class witness_vote_object;
class operation_object;
class account_history_object;
class hardfork_property_object;
class withdraw_vesting_route_object;
class owner_authority_history_object;
class account_recovery_request_object;
class change_recovery_account_request_object;
class escrow_object;
class decline_voting_rights_request_object;
class block_stats_object;
class reward_fund_object;
class vesting_delegation_object;
class vesting_delegation_expiration_object;
class budget_object;
class budget_with_schedule_object;
class budget_schedule_object;

typedef oid<dynamic_global_property_object> dynamic_global_property_id_type;
typedef oid<chain_property_object> chain_property_id_type;
typedef oid<account_object> account_id_type;
typedef oid<account_authority_object> account_authority_id_type;
typedef oid<witness_object> witness_id_type;
typedef oid<transaction_object> transaction_object_id_type;
typedef oid<block_summary_object> block_summary_id_type;
typedef oid<witness_schedule_object> witness_schedule_id_type;
typedef oid<comment_object> comment_id_type;
typedef oid<comment_vote_object> comment_vote_id_type;
typedef oid<witness_vote_object> witness_vote_id_type;
typedef oid<operation_object> operation_id_type;
typedef oid<account_history_object> account_history_id_type;
typedef oid<hardfork_property_object> hardfork_property_id_type;
typedef oid<withdraw_vesting_route_object> withdraw_vesting_route_id_type;
typedef oid<owner_authority_history_object> owner_authority_history_id_type;
typedef oid<account_recovery_request_object> account_recovery_request_id_type;
typedef oid<change_recovery_account_request_object> change_recovery_account_request_id_type;
typedef oid<escrow_object> escrow_id_type;
typedef oid<decline_voting_rights_request_object> decline_voting_rights_request_id_type;
typedef oid<block_stats_object> block_stats_id_type;
typedef oid<reward_fund_object> reward_fund_id_type;
typedef oid<vesting_delegation_object> vesting_delegation_id_type;
typedef oid<vesting_delegation_expiration_object> vesting_delegation_expiration_id_type;
typedef oid<budget_object> budget_id_type;
typedef oid<budget_with_schedule_object> budget_with_schedule_id_type;
typedef oid<budget_schedule_object> budget_schedule_id_type;

enum bandwidth_type
{
    post, ///< Rate limiting posting reward eligibility over time
    forum, ///< Rate limiting for all forum related actins
    market ///< Rate limiting for all other actions
};
}
} // scorum::chain

namespace fc {
class variant;
inline void to_variant(const scorum::chain::shared_string& s, variant& var)
{
    var = fc::string(scorum::chain::to_string(s));
}

inline void from_variant(const variant& var, scorum::chain::shared_string& s)
{
    auto str = var.as_string();
    s.assign(str.begin(), str.end());
}

template <typename T> void to_variant(const chainbase::oid<T>& var, variant& vo)
{
    vo = var._id;
}
template <typename T> void from_variant(const variant& vo, chainbase::oid<T>& var)
{
    var._id = vo.as_int64();
}

namespace raw {
template <typename Stream, typename T> inline void pack(Stream& s, const chainbase::oid<T>& id)
{
    s.write((const char*)&id._id, sizeof(id._id));
}
template <typename Stream, typename T> inline void unpack(Stream& s, chainbase::oid<T>& id)
{
    s.read((char*)&id._id, sizeof(id._id));
}
}

namespace raw {
namespace bip = chainbase::bip;
using chainbase::allocator;

template <typename T> inline void pack(scorum::chain::buffer_type& raw, const T& v)
{
    auto size = pack_size(v);
    raw.resize(size);
    datastream<char*> ds(raw.data(), size);
    pack(ds, v);
}

template <typename T> inline void unpack(const scorum::chain::buffer_type& raw, T& v)
{
    datastream<const char*> ds(raw.data(), raw.size());
    unpack(ds, v);
}

template <typename T> inline T unpack(const scorum::chain::buffer_type& raw)
{
    T v;
    datastream<const char*> ds(raw.data(), raw.size());
    unpack(ds, v);
    return v;
}
}
}

namespace fc {
}

// clang-format off

FC_REFLECT_ENUM( scorum::chain::object_type,
                 (dynamic_global_property_object_type)
                 (chain_property_object_type)
                 (account_object_type)
                 (account_authority_object_type)
                 (witness_object_type)
                 (transaction_object_type)
                 (block_summary_object_type)
                 (witness_schedule_object_type)
                 (comment_object_type)
                 (comment_vote_object_type)
                 (witness_vote_object_type)
                 (operation_object_type)
                 (account_history_object_type)
                 (hardfork_property_object_type)
                 (withdraw_vesting_route_object_type)
                 (owner_authority_history_object_type)
                 (account_recovery_request_object_type)
                 (change_recovery_account_request_object_type)
                 (escrow_object_type)
                 (decline_voting_rights_request_object_type)
                 (block_stats_object_type)
                 (reward_fund_object_type)
                 (vesting_delegation_object_type)
                 (vesting_delegation_expiration_object_type)
                 (budget_object_type)
                 (budget_with_schedule_object_type)
                 (budget_schedule_object_type)
                 )

FC_REFLECT_TYPENAME( scorum::chain::shared_string )
FC_REFLECT_TYPENAME( scorum::chain::buffer_type )

FC_REFLECT_ENUM( scorum::chain::bandwidth_type, (post)(forum)(market) )

// clang-format on
