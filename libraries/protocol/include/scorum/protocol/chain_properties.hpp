#pragma once
#include <scorum/protocol/asset.hpp>
#include <scorum/protocol/config.hpp>

namespace scorum {
namespace protocol {

/**
* Witnesses must vote on how to set certain chain properties to ensure a smooth
* and well functioning network.  Any time @owner is in the active set of witnesses these
* properties will be used to control the blockchain configuration.
*/
struct chain_properties
{
    /**
    *  This fee, paid in SCR, is converted into SP for the new account. Accounts
    *  without scorumpower cannot earn usage rations and therefore are powerless. This minimum
    *  fee requires all accounts to have some kind of commitment to the network that includes the
    *  ability to vote and make transactions.
    */
    asset account_creation_fee = SCORUM_MIN_ACCOUNT_CREATION_FEE;

    /**
    *  This witnesses vote for the maximum_block_size which is used by the network
    *  to tune rate limiting and capacity
    */
    uint32_t maximum_block_size = SCORUM_MIN_BLOCK_SIZE_LIMIT * 2;

    void validate() const
    {
        FC_ASSERT(account_creation_fee >= SCORUM_MIN_ACCOUNT_CREATION_FEE);
        FC_ASSERT(maximum_block_size >= SCORUM_MIN_BLOCK_SIZE_LIMIT);
    }
};
}
}

FC_REFLECT(scorum::protocol::chain_properties, (account_creation_fee)(maximum_block_size))
