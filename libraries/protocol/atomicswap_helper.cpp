#include <scorum/protocol/atomicswap_helper.hpp>

#include <fc/crypto/ripemd160.hpp>
#include <sstream>

namespace scorum {
namespace protocol {
namespace atomicswap {

void validate_secret_hash(const std::string& secret_hash)
{
    fc::ripemd160 fmt;
    FC_ASSERT(secret_hash.size() == fmt.data_size() * 2, "Invalid hash format. It must be in hex RIPEMD160 format");
    try
    {
        fmt = fc::ripemd160(secret_hash);
    }
    FC_CAPTURE_AND_RETHROW((secret_hash))
}

std::string get_secret_hash(const std::string& secret)
{
    fc::ripemd160 encode;
    return encode.hash(secret).str();
}

fc::sha256 get_contract_hash_obj(const account_name_type& recipient, const std::string& secret_hash)
{
    std::stringstream data;
    data << recipient << secret_hash;
    return fc::sha256().hash(data.str());
}

std::string get_contract_hash_hex(const account_name_type& recipient, const std::string& secret_hash)
{
    return get_contract_hash_obj(recipient, secret_hash).str();
}

hash_index_type get_contract_hash(const account_name_type& recipient, const std::string& secret_hash)
{
    fc::sha256 encode = get_contract_hash_obj(recipient, secret_hash);

    // pack 256-bit binary data to 32-bt fixed string
    hash_index_type ret;
    // check if somebody change hash index_type size or sha256 impl
    FC_ASSERT(encode.data_size() == sizeof(ret.data));
    memcpy((char*)&ret.data, encode.data(), encode.data_size());

    ret.data = boost::endian::big_to_native(ret.data);

    return ret;
}
}
}
}
