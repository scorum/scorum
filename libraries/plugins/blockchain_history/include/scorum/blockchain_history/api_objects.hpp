#pragma once

#include <scorum/protocol/block.hpp>

#include <scorum/blockchain_history/schema/applied_operation.hpp>

namespace scorum {
namespace blockchain_history {

using namespace protocol;

struct signed_block_api_obj : public signed_block
{
    signed_block_api_obj(const signed_block& block)
        : signed_block(block)
    {
        block_id = id();
        signing_key = signee();
        transaction_ids.reserve(transactions.size());
        for (const signed_transaction& tx : transactions)
            transaction_ids.push_back(tx.id());
    }
    signed_block_api_obj()
    {
    }

    block_id_type block_id;
    public_key_type signing_key;
    std::vector<transaction_id_type> transaction_ids;
};

struct block_api_operation_object
{
    block_api_operation_object() = default;

    block_api_operation_object(const applied_operation& op)
    {
        trx_id = op.trx_id;
        timestamp = op.timestamp;
        op.op.visit([&](const auto& operation) { this->op = operation; });
    }

    transaction_id_type trx_id;
    fc::time_point_sec timestamp;
    operation op;
};

struct block_api_object : public signed_block_header
{
    block_api_object() = default;

    block_api_object(const signed_block_header& b)
        : signed_block_header(b)
    {
    }

    uint32_t block_num = 0;
    std::vector<block_api_operation_object> operations;
};
}
}

FC_REFLECT(scorum::blockchain_history::block_api_operation_object, (trx_id)(timestamp)(op))

FC_REFLECT_DERIVED(scorum::blockchain_history::block_api_object,
                   (scorum::protocol::signed_block_header),
                   (block_num)(operations))

FC_REFLECT_DERIVED(scorum::blockchain_history::signed_block_api_obj,
                   (scorum::protocol::signed_block),
                   (block_id)(signing_key)(transaction_ids))
