#pragma once

#include <fc/container/flat_fwd.hpp>
#include <fc/io/varint.hpp>
#include <fc/io/enum_type.hpp>
#include <fc/crypto/sha224.hpp>
#include <fc/crypto/ripemd160.hpp>
#include <fc/crypto/elliptic.hpp>
#include <fc/reflect/reflect.hpp>
#include <fc/reflect/variant.hpp>
#include <fc/optional.hpp>
#include <fc/safe.hpp>
#include <fc/container/flat.hpp>
#include <fc/string.hpp>
#include <fc/fixed_string.hpp>
#include <fc/io/raw.hpp>
#include <fc/uint128.hpp>
#include <fc/static_variant.hpp>
#include <fc/smart_ref_fwd.hpp>
#include <fc/time.hpp>
#include <fc/safe.hpp>

#include <boost/multiprecision/cpp_int.hpp>

#include <memory>
#include <vector>
#include <deque>
#include <cstdint>

namespace scorum {

using fc::uint128_t;
typedef boost::multiprecision::uint256_t u256;
typedef boost::multiprecision::uint512_t u512;

using fc::enum_type;
using fc::flat_map;
using fc::flat_set;
using fc::optional;
using fc::safe;
using fc::signed_int;
using fc::smart_ref;
using fc::static_variant;
using fc::time_point;
using fc::time_point_sec;
using fc::unsigned_int;
using fc::variant;
using fc::variant_object;
using fc::ecc::commitment_type;
using fc::ecc::range_proof_info;
using fc::ecc::range_proof_type;
struct void_t
{
};

namespace protocol {

typedef fc::ecc::private_key private_key_type;
typedef fc::sha256 chain_id_type;
typedef fc::fixed_string_16 account_name_type;
typedef fc::ripemd160 block_id_type;
typedef fc::ripemd160 checksum_type;
typedef fc::ripemd160 transaction_id_type;
typedef fc::sha256 digest_type;
typedef fc::ecc::compact_signature signature_type;
typedef uint16_t authority_weight_type;
typedef uint16_t percent_type;
typedef int16_t vote_weight_type;

using share_value_type = int64_t;
using share_type = fc::safe<share_value_type>;

struct public_key_type
{
    struct binary_key
    {
        binary_key()
        {
        }
        uint32_t check = 0;
        fc::ecc::public_key_data data;
    };
    fc::ecc::public_key_data key_data;
    public_key_type();
    public_key_type(const fc::ecc::public_key_data& data);
    public_key_type(const fc::ecc::public_key& pubkey);
    explicit public_key_type(const std::string& base58str);
    operator fc::ecc::public_key_data() const;
    operator fc::ecc::public_key() const;
    explicit operator std::string() const;
    friend bool operator==(const public_key_type& p1, const fc::ecc::public_key& p2);
    friend bool operator==(const public_key_type& p1, const public_key_type& p2);
    friend bool operator<(const public_key_type& p1, const public_key_type& p2)
    {
        return p1.key_data < p2.key_data;
    }
    friend bool operator!=(const public_key_type& p1, const public_key_type& p2);
};

struct extended_public_key_type
{
    struct binary_key
    {
        binary_key()
        {
        }
        uint32_t check = 0;
        fc::ecc::extended_key_data data;
    };

    fc::ecc::extended_key_data key_data;

    extended_public_key_type();
    extended_public_key_type(const fc::ecc::extended_key_data& data);
    extended_public_key_type(const fc::ecc::extended_public_key& extpubkey);
    explicit extended_public_key_type(const std::string& base58str);
    operator fc::ecc::extended_public_key() const;
    explicit operator std::string() const;
    friend bool operator==(const extended_public_key_type& p1, const fc::ecc::extended_public_key& p2);
    friend bool operator==(const extended_public_key_type& p1, const extended_public_key_type& p2);
    friend bool operator!=(const extended_public_key_type& p1, const extended_public_key_type& p2);
};

struct extended_private_key_type
{
    struct binary_key
    {
        binary_key()
        {
        }
        uint32_t check = 0;
        fc::ecc::extended_key_data data;
    };

    fc::ecc::extended_key_data key_data;

    extended_private_key_type();
    extended_private_key_type(const fc::ecc::extended_key_data& data);
    extended_private_key_type(const fc::ecc::extended_private_key& extprivkey);
    explicit extended_private_key_type(const std::string& base58str);
    operator fc::ecc::extended_private_key() const;
    explicit operator std::string() const;
    friend bool operator==(const extended_private_key_type& p1, const fc::ecc::extended_private_key& p2);
    friend bool operator==(const extended_private_key_type& p1, const extended_private_key_type& p2);
    friend bool operator!=(const extended_private_key_type& p1, const extended_private_key_type& p2);
};

enum class curve_id
{
    quadratic,
    linear,
    square_root,
    power1dot5
};

enum class budget_type
{
    post,
    banner
};
} // namespace protocol
} // namespace scorum

namespace fc {
void to_variant(const scorum::protocol::public_key_type& var, fc::variant& vo);
void from_variant(const fc::variant& var, scorum::protocol::public_key_type& vo);
void to_variant(const scorum::protocol::extended_public_key_type& var, fc::variant& vo);
void from_variant(const fc::variant& var, scorum::protocol::extended_public_key_type& vo);
void to_variant(const scorum::protocol::extended_private_key_type& var, fc::variant& vo);
void from_variant(const fc::variant& var, scorum::protocol::extended_private_key_type& vo);
} // namespace fc

FC_REFLECT(scorum::protocol::public_key_type, (key_data))
FC_REFLECT(scorum::protocol::public_key_type::binary_key, (data)(check))
FC_REFLECT(scorum::protocol::extended_public_key_type, (key_data))
FC_REFLECT(scorum::protocol::extended_public_key_type::binary_key, (check)(data))
FC_REFLECT(scorum::protocol::extended_private_key_type, (key_data))
FC_REFLECT(scorum::protocol::extended_private_key_type::binary_key, (check)(data))

FC_REFLECT_ENUM(scorum::protocol::curve_id, (quadratic)(linear)(square_root)(power1dot5))
FC_REFLECT_ENUM(scorum::protocol::budget_type, (post)(banner))

FC_REFLECT(scorum::void_t, )
