#include <scorum/chain/dbs_atomicswap.hpp>
#include <scorum/chain/database.hpp>
#include <scorum/chain/dbs_account.hpp>
#include <scorum/chain/account_object.hpp>
#include <scorum/protocol/atomicswap_helper.hpp>

using namespace scorum::protocol;

namespace scorum {
namespace chain {

dbs_atomicswap::dbs_atomicswap(database& db)
    : _base_type(db)
{
}

dbs_atomicswap::atomicswap_contracts_refs_type dbs_atomicswap::get_contracts(const account_name_type& owner) const
{
    atomicswap_contracts_refs_type ret;

    const auto& idx = db_impl().get_index<atomicswap_contract_index>().indicies().get<by_owner_name>();
    for (auto it = idx.cbegin(); it != idx.cend(); ++it)
    {
        ret.push_back(std::cref(*it));
    }

    return ret;
}

const atomicswap_contract_object& dbs_atomicswap::get_contract(const account_object& recipient,
                                                               const std::string& secret_hash) const
{
    FC_ASSERT(!secret_hash.empty(), "Empty secret hash.");

    atomicswap::hash_index_type contract_hash = atomicswap::get_contract_hash(recipient.name, secret_hash);
    return db_impl().get<atomicswap_contract_object, by_contract_hash>(contract_hash);
}

const atomicswap_contract_object& dbs_atomicswap::create_initiator_contract(const account_object& initiator,
                                                                            const account_object& participant,
                                                                            const asset& amount,
                                                                            const std::string& secret_hash)
{
    FC_ASSERT(amount.symbol == SCORUM_SYMBOL, "Invalid asset type (symbol).");
    FC_ASSERT(amount.amount > 0, "Invalid amount.");
    FC_ASSERT(initiator.balance >= amount, "Insufficient funds.");
    FC_ASSERT(!secret_hash.empty(), "Empty secret hash.");

    atomicswap::hash_index_type contract_hash = atomicswap::get_contract_hash(participant.name, secret_hash);

    FC_ASSERT((nullptr == db_impl().find<atomicswap_contract_object, by_contract_hash>(contract_hash)),
              "There is contract for '${a}' with same secret. Use another secret and try again.",
              ("a", participant.name));

    const dynamic_global_property_object& props = db_impl().get_dynamic_global_properties();

    time_point_sec start = props.time;
    time_point_sec deadline = start + fc::seconds(SCORUM_ATOMICSWAP_INITIATOR_REFUND_LOCK_SECS);

    const atomicswap_contract_object& new_contract
        = db_impl().create<atomicswap_contract_object>([&](atomicswap_contract_object& contract) {
              contract.owner = initiator.name;
              contract.to = participant.name;
              contract.amount = amount;
              contract.created = start;
              contract.deadline = deadline;
              fc::from_string(contract.secret_hash, secret_hash);
              contract.contract_hash = contract_hash;
          });

    dbs_account& account_service = db().obtain_service<dbs_account>();
    account_service.lock_balance(initiator, amount);

    return new_contract;
}

void dbs_atomicswap::redeem_contract(const atomicswap_contract_object& contract)
{
    dbs_account& account_service = db().obtain_service<dbs_account>();

    const account_object& from = account_service.get_account(contract.owner);
    const account_object& to = account_service.get_account(contract.to);

    FC_ASSERT(contract.amount == account_service.unlock_balance(from, contract.amount), "Insufficient funds.");
    account_service.decrease_balance(from, contract.amount);
    account_service.increase_balance(to, contract.amount);

    db_impl().remove(contract);
}

void dbs_atomicswap::refund_contract(const atomicswap_contract_object& contract)
{
    dbs_account& account_service = db().obtain_service<dbs_account>();

    const account_object& owner = account_service.get_account(contract.owner);

    account_service.unlock_balance(owner, contract.amount);

    db_impl().remove(contract);
}

void dbs_atomicswap::check_contracts_expiration()
{
    atomicswap_contracts_refs_type contracts;

    const auto& idx = db_impl().get_index<atomicswap_contract_index>().indicies().get<by_owner_name>();
    for (auto it = idx.cbegin(); it != idx.cend(); ++it)
    {
        contracts.push_back(std::cref(*it));
    }

    const dynamic_global_property_object& props = db_impl().get_dynamic_global_properties();

    for (const atomicswap_contract_object& contract : contracts)
    {
        if (props.time >= contract.deadline)
        {
            refund_contract(contract);
        }
    }
}

} // namespace chain
} // namespace scorum
