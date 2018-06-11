#pragma once
#include <scorum/protocol/block_header.hpp>
#include <scorum/protocol/transaction.hpp>

#include <string>

namespace scorum {
namespace protocol {

struct signed_block : public signed_block_header
{
    checksum_type calculate_merkle_root() const;
    std::vector<signed_transaction> transactions;
};
}

// use for context in logs
class block_info
{
public:
    block_info(const scorum::protocol::signed_block&);
    block_info(const fc::time_point_sec& when, const std::string& witness_owner);
    block_info()
    {
    }

    operator std::string() const;

private:
    uint32_t _block_num = 0;
    std::string _block_id = scorum::protocol::digest_type().str();
    fc::time_point_sec _when;
    std::string _block_witness = "?";
};

} // scorum::protocol

FC_REFLECT_DERIVED(scorum::protocol::signed_block, (scorum::protocol::signed_block_header), (transactions))
