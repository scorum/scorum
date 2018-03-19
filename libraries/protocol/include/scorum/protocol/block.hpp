#pragma once
#include <scorum/protocol/block_header.hpp>
#include <scorum/protocol/transaction.hpp>

namespace scorum {
namespace protocol {

struct signed_block : public signed_block_header
{
    checksum_type calculate_merkle_root() const;
    std::vector<signed_transaction> transactions;
};
}
} // scorum::protocol

FC_REFLECT_DERIVED(scorum::protocol::signed_block, (scorum::protocol::signed_block_header), (transactions))
