#pragma once

#include <scorum/chain/dbs_base_impl.hpp>
#include <vector>
#include <functional>

#include <scorum/chain/atomicswap_objects.hpp>

namespace scorum {
namespace chain {

/** DB service for operations with atomicswap_contract_object
 *  --------------------------------------------
 */
class dbs_atomicswap : public dbs_base
{
    friend class dbservice_dbs_factory;

protected:
    explicit dbs_atomicswap(database& db);

public:
    using atomicswap_contracts_refs_type = std::vector<std::reference_wrapper<const atomicswap_contract_object>>;

    atomicswap_contracts_refs_type get_contracts(const account_object& owner) const;

    const atomicswap_contract_object& get_contract(const account_object& recipient,
                                                   const std::string& secret_hash) const;

    const atomicswap_contract_object& create_initiator_contract(const account_object& initiator,
                                                                const account_object& participant,
                                                                const asset& amount,
                                                                const std::string& secret_hash);

    void redeem_contract(const atomicswap_contract_object& contract);

    void refund_contract(const atomicswap_contract_object& contract);

    void check_contracts_expiration();
};
} // namespace chain
} // namespace scorum
