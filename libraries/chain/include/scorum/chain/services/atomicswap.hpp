#pragma once

#include <scorum/chain/services/service_base.hpp>
#include <scorum/chain/schema/atomicswap_objects.hpp>

namespace scorum {
namespace chain {

class account_object;
enum atomicswap_contract_type : bool;

struct atomicswap_service_i : public base_service_i<atomicswap_contract_object>
{
    using atomicswap_contracts_refs_type = std::vector<std::reference_wrapper<const atomicswap_contract_object>>;

    virtual atomicswap_contracts_refs_type get_contracts(const account_object& owner) const = 0;

    virtual const atomicswap_contract_object&
    get_contract(const account_object& from, const account_object& to, const std::string& secret_hash) const = 0;

    virtual const atomicswap_contract_object& create_contract(atomicswap_contract_type tp,
                                                              const account_object& owner,
                                                              const account_object& recipient,
                                                              const asset& amount,
                                                              const std::string& secret_hash,
                                                              const optional<std::string>& metadata
                                                              = optional<std::string>())
        = 0;

    virtual void redeem_contract(const atomicswap_contract_object& contract, const std::string& secret) = 0;
    virtual void refund_contract(const atomicswap_contract_object& contract) = 0;
    virtual void check_contracts_expiration() = 0;
};

/**
 * DB service for operations with atomicswap_contract_object
 */
class dbs_atomicswap : public dbs_service_base<atomicswap_service_i>
{
    friend class dbservice_dbs_factory;

protected:
    explicit dbs_atomicswap(database& db);

public:
    virtual atomicswap_contracts_refs_type get_contracts(const account_object& owner) const override;

    virtual const atomicswap_contract_object&
    get_contract(const account_object& from, const account_object& to, const std::string& secret_hash) const override;

    virtual const atomicswap_contract_object& create_contract(atomicswap_contract_type tp,
                                                              const account_object& owner,
                                                              const account_object& recipient,
                                                              const asset& amount,
                                                              const std::string& secret_hash,
                                                              const optional<std::string>& metadata
                                                              = optional<std::string>()) override;

    virtual void redeem_contract(const atomicswap_contract_object& contract, const std::string& secret) override;

    virtual void refund_contract(const atomicswap_contract_object& contract) override;

    virtual void check_contracts_expiration() override;

private:
    std::size_t _contracts_per_recipient(const account_name_type& owner, const account_name_type& recipient) const;
};
} // namespace chain
} // namespace scorum
