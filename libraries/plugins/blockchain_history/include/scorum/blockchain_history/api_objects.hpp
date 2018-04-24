#pragma once

#include <scorum/protocol/block.hpp>

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
}
}

FC_REFLECT_DERIVED(scorum::blockchain_history::signed_block_api_obj,
                   (scorum::protocol::signed_block),
                   (block_id)(signing_key)(transaction_ids))
