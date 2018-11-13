#include <scorum/chain/database/database.hpp>
#include <scorum/chain/services/atomicswap.hpp>
#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/chain/schema/account_objects.hpp>
#include <scorum/chain/schema/atomicswap_objects.hpp>

#include <scorum/protocol/atomicswap_helper.hpp>

using namespace scorum::protocol;

namespace scorum {
namespace chain {

dbs_atomicswap::dbs_atomicswap(database& db)
    : base_service_type(db)
    , _dgp_svc(db.dynamic_global_property_service())
    , _account_svc(db.account_service())
{
}

dbs_atomicswap::atomicswap_contracts_refs_type dbs_atomicswap::get_contracts() const
{
    atomicswap_contracts_refs_type ret;

    const auto& idx = db_impl().get_index<atomicswap_contract_index>().indices().get<by_owner_name>();

    std::copy(idx.cbegin(), idx.cend(), std::back_inserter(ret));

    return ret;
}

dbs_atomicswap::atomicswap_contracts_refs_type dbs_atomicswap::get_contracts(const account_object& owner) const
{
    atomicswap_contracts_refs_type ret;

    const auto& idx
        = db_impl().get_index<atomicswap_contract_index>().indices().get<by_owner_name>().equal_range(owner.name);
    for (auto it = idx.first; it != idx.second; ++it)
    {
        ret.push_back(std::cref(*it));
    }

    return ret;
}

const atomicswap_contract_object&
dbs_atomicswap::get_contract(const account_object& from, const account_object& to, const std::string& secret_hash) const
{
    FC_ASSERT(!secret_hash.empty(), "Empty secret hash.");

    try
    {
        atomicswap::hash_index_type contract_hash = atomicswap::get_contract_hash(from.name, to.name, secret_hash);
        return get_by<by_contract_hash>(contract_hash);
    }
    FC_CAPTURE_AND_RETHROW((from.name)(to.name)(secret_hash))
}

const atomicswap_contract_object& dbs_atomicswap::create_contract(atomicswap_contract_type tp,
                                                                  const account_object& owner,
                                                                  const account_object& recipient,
                                                                  const asset& amount,
                                                                  const std::string& secret_hash,
                                                                  const optional<std::string>& metadata)
{
    FC_ASSERT(amount.symbol() == SCORUM_SYMBOL, "Invalid asset type (symbol).");
    FC_ASSERT(amount.amount > 0, "Invalid amount.");
    FC_ASSERT(owner.balance >= amount, "Insufficient funds.");
    FC_ASSERT(!secret_hash.empty(), "Empty secret hash.");

    atomicswap::hash_index_type contract_hash = atomicswap::get_contract_hash(owner.name, recipient.name, secret_hash);

    FC_ASSERT((nullptr == find_by<by_contract_hash>(contract_hash)),
              "There is contract for '${a}' with same secret. Use another secret and try again.",
              ("a", recipient.name));
    FC_ASSERT(get_contracts(owner).size() < SCORUM_ATOMICSWAP_LIMIT_REQUESTED_CONTRACTS_PER_OWNER,
              "Can't create more then ${1} contract per owner.",
              ("1", SCORUM_ATOMICSWAP_LIMIT_REQUESTED_CONTRACTS_PER_OWNER));
    FC_ASSERT(_contracts_per_recipient(owner.name, recipient.name)
                  < (std::size_t)SCORUM_ATOMICSWAP_LIMIT_REQUESTED_CONTRACTS_PER_RECIPIENT,
              "Can't create more then ${1} contract per recipient.",
              ("1", SCORUM_ATOMICSWAP_LIMIT_REQUESTED_CONTRACTS_PER_RECIPIENT));

    time_point_sec start = _dgp_svc.head_block_time();
    time_point_sec deadline = start;
    switch (tp)
    {
    case atomicswap_contract_initiator:
        deadline += fc::seconds(SCORUM_ATOMICSWAP_INITIATOR_REFUND_LOCK_SECS);
        break;
    case atomicswap_contract_participant:
        deadline += fc::seconds(SCORUM_ATOMICSWAP_PARTICIPANT_REFUND_LOCK_SECS);
        break;
    default:
        FC_ASSERT(false, "Invalid contract type.");
    }

    const atomicswap_contract_object& new_contract = create([&](atomicswap_contract_object& contract) {
        contract.type = tp;
        contract.owner = owner.name;
        contract.to = recipient.name;
        contract.amount = amount;
        contract.created = start;
        contract.deadline = deadline;
        fc::from_string(contract.secret_hash, secret_hash);
        contract.contract_hash = contract_hash;
        if (metadata.valid())
        {
            fc::from_string(contract.metadata, *metadata);
        }
    });

    _account_svc.decrease_balance(owner, amount);

    return new_contract;
}

void dbs_atomicswap::redeem_contract(const atomicswap_contract_object& contract, const std::string& secret)
{
    const account_object& to = _account_svc.get_account(contract.to);

    _account_svc.increase_balance(to, contract.amount);

    if (contract.type == atomicswap_contract_initiator)
    {
        remove(contract);
    }
    else
    {
        // save secret for participant
        update(contract, [&](atomicswap_contract_object& contract) {
            fc::from_string(contract.secret, secret);
            contract.amount.amount = 0; // asset moved to 'to' balance
        });
    }
}

void dbs_atomicswap::refund_contract(const atomicswap_contract_object& contract)
{
    const account_object& owner = _account_svc.get_account(contract.owner);

    _account_svc.increase_balance(owner, contract.amount);

    remove(contract);
}

std::size_t dbs_atomicswap::_contracts_per_recipient(const account_name_type& owner,
                                                     const account_name_type& recipient) const
{
    return db_impl().get_index<atomicswap_contract_index>().indices().get<by_recipient_name>().count(recipient);
}

} // namespace chain
} // namespace scorum
