#pragma once

#include <string>
#include <scorum/protocol/types.hpp>

namespace scorum {
namespace wallet {

namespace sp = scorum::protocol;

struct brain_key_info
{
    std::string brain_priv_key;
    sp::public_key_type pub_key;
    std::string wif_priv_key;
};

std::string pubkey_to_shorthash(const sp::public_key_type& key);
std::string normalize_brain_key(const std::string& s);
fc::ecc::private_key derive_private_key(const std::string& prefix_string, int sequence_number);
brain_key_info suggest_brain_key();

} // namespace scorum
} // namespace wallet
