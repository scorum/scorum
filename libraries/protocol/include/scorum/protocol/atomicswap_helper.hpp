#pragma once

#include <scorum/protocol/types.hpp>

#include <string>
#include <fc/crypto/sha256.hpp>

namespace scorum {
namespace protocol {
namespace atomicswap {

using hash_index_type = fc::fixed_string_32; // to pack 256-bit hash

std::string get_secret_packed(const std::string& secret);
void validate_secret_hash(const std::string& secret_hash);
std::string get_secret_hash(const std::string& secret);
fc::sha256
get_contract_hash_obj(const account_name_type& from, const account_name_type& to, const std::string& secret_hash);
std::string
get_contract_hash_hex(const account_name_type& from, const account_name_type& to, const std::string& secret_hash);
hash_index_type
get_contract_hash(const account_name_type& from, const account_name_type& to, const std::string& secret_hash);
}
}
}
