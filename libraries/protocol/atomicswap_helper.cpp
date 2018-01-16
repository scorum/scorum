#include <scorum/protocol/atomicswap_helper.hpp>

#include <fc/crypto/sha512.hpp>
#include <fc/crypto/hex.hpp>
#include <fc/crypto/ripemd160.hpp>

#include <sstream>

namespace scorum {
namespace protocol {
namespace atomicswap {

std::string get_secret_hex(const std::string& secret, const uint8_t secret_length /*= 0*/)
{
    fc::sha512 hash = fc::sha512().hash(secret);
    FC_ASSERT(secret_length <= hash.data_size(), "Required secret string is too long. Use length less or equal ${1}",
              ("1", hash.data_size()));
    std::size_t out_sz = secret_length;
    if (!out_sz)
    {
        static const size_t entropy_percent = (std::size_t)50;
        FC_ASSERT(entropy_percent > 0);
        std::size_t entropy = secret.size();
        // get value in [entropy_percent, 2*entropy_percent - 1]
        entropy = entropy_percent + entropy % entropy_percent;
        ++entropy; // increase value for [entropy_percent + 1, 2*entropy_percent]
        // devide to 2*entropy_percent to get coefficient in (0.5, 1]
        out_sz = hash.data_size() * entropy / entropy_percent / (std::size_t)2;
        if (out_sz > SCORUM_ATOMICSWAP_SECRET_MAX_LENGTH / 2)
        {
            // Ensure out hex string less then SCORUM_ATOMICSWAP_SECRET_MAX_LENGTH (length in hex view (two symbol per
            // byte))
            out_sz = SCORUM_ATOMICSWAP_SECRET_MAX_LENGTH / 2;
        }
    }
    return fc::to_hex(hash.data(), out_sz);
}

std::string get_secret_hash(const std::string& secret_hex)
{
    fc::ripemd160::encoder e;
    char ch_out{ '\0' };
    bool odd = false;
    for (char ch : secret_hex)
    {
        odd = !odd;
        if (odd)
        {
            ch_out = fc::from_hex(ch) << 4;
        }
        else
        {
            ch_out |= fc::from_hex(ch);
            e.put(ch_out);
        }
    }
    if (odd)
    {
        // if secret_hex is not even 1 byte
        e.put(ch_out);
    }
    fc::ripemd160 encode = e.result();
    return encode.str();
}

void validate_secret(const std::string& secret_hex)
{
    FC_ASSERT(!secret_hex.empty(), "Empty secret.");
    FC_ASSERT(secret_hex.size() < SCORUM_ATOMICSWAP_SECRET_MAX_LENGTH, "Secret string is too long.");
    for (char ch : secret_hex)
    {
        fc::from_hex(ch);
    }
}

void validate_secret_hash(const std::string& secret_hash)
{
    fc::ripemd160 fmt;
    FC_ASSERT(secret_hash.size() == fmt.data_size() * 2, "Invalid hash format. It must be in hex RIPEMD160 format.");
    try
    {
        fmt = fc::ripemd160(secret_hash);
    }
    FC_CAPTURE_AND_RETHROW((secret_hash))
}

void validate_contract_metadata(const std::string& metadata)
{
    FC_ASSERT(metadata.size() < SCORUM_ATOMICSWAP_CONTRACT_METADATA_MAX_LENGTH,
              "Contract metadata string is too long.");
}

fc::sha256
get_contract_hash_obj(const account_name_type& from, const account_name_type& to, const std::string& secret_hash)
{
    std::stringstream store;
    store << from << to << secret_hash;
    return fc::sha256().hash(store.str());
}

std::string
get_contract_hash_hex(const account_name_type& from, const account_name_type& to, const std::string& secret_hash)
{
    return get_contract_hash_obj(from, to, secret_hash).str();
}

hash_index_type
get_contract_hash(const account_name_type& from, const account_name_type& to, const std::string& secret_hash)
{
    fc::sha256 encode = get_contract_hash_obj(from, to, secret_hash);

    // pack 256-bit binary data to 32-bt fixed string
    hash_index_type ret;
    // check if somebody change hash index_type size or sha256 impl
    FC_ASSERT(encode.data_size() == sizeof(ret.data));
    memcpy((char*)&ret.data, encode.data(), encode.data_size());

    ret.data = boost::endian::big_to_native(ret.data);

#ifdef IS_TEST_NET
    std::stringstream print;
    print << "Contract hash for ";
    print << from << "," << to << "," << secret_hash << " = ";
    print << get_contract_hash_hex(from, to, secret_hash);
    ulog(print.str());
#endif
    return ret;
}
}
}
}
