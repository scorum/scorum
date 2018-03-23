#pragma once

#include <scorum/protocol/base.hpp>
#include <scorum/protocol/types.hpp>
#include <scorum/protocol/asset.hpp>
#include <scorum/protocol/operation_util.hpp>
#include <fc/static_variant.hpp>

namespace scorum {
namespace protocol {

enum quorum_type
{
    none_quorum,
    add_member_quorum,
    exclude_member_quorum,
    base_quorum,
    transfer_quorum
};

inline void validate_quorum(quorum_type t, protocol::percent_type quorum)
{
    FC_ASSERT(t != none_quorum, "Quorum type is not set.");
    FC_ASSERT(quorum >= SCORUM_MIN_QUORUM_VALUE_PERCENT, "Quorum is to small.");
    FC_ASSERT(quorum <= SCORUM_MAX_QUORUM_VALUE_PERCENT, "Quorum is to large.");
}

struct committee_i
{
    virtual void add_member(const account_name_type&) = 0;
    virtual void exclude_member(const account_name_type&) = 0;

    virtual void change_add_member_quorum(const protocol::percent_type) = 0;
    virtual void change_exclude_member_quorum(const protocol::percent_type) = 0;
    virtual void change_base_quorum(const protocol::percent_type) = 0;
    virtual void change_transfer_quorum(const protocol::percent_type) = 0;

    virtual protocol::percent_type get_add_member_quorum() = 0;
    virtual protocol::percent_type get_exclude_member_quorum() = 0;
    virtual protocol::percent_type get_base_quorum() = 0;
    virtual protocol::percent_type get_transfer_quorum() = 0;

    virtual bool is_exists(const account_name_type&) const = 0;
    virtual size_t get_members_count() const = 0;
};

struct registration_committee_i : public committee_i
{
};

struct development_committee_i : public committee_i
{
};

template <typename CommitteeType> struct proposal_committee_operation
{
    typedef CommitteeType committee_type;
    typedef proposal_committee_operation<CommitteeType> committee_operation_type;
};

template <typename OperationType, typename CommitteeType>
struct proposal_base_operation : public proposal_committee_operation<CommitteeType>
{
    typedef OperationType operation_type;
};

struct registration_committee_add_member_operation
    : public proposal_base_operation<registration_committee_add_member_operation, registration_committee_i>
{
    account_name_type account_name;

    void validate() const;

    protocol::percent_type get_required_quorum(committee_i& committee_service) const;
};

struct registration_committee_exclude_member_operation
    : public proposal_base_operation<registration_committee_exclude_member_operation, registration_committee_i>
{
    account_name_type account_name;

    void validate() const;

    protocol::percent_type get_required_quorum(committee_i& committee_service) const;
};

struct registration_committee_change_quorum_operation
    : public proposal_base_operation<registration_committee_change_quorum_operation, registration_committee_i>
{
    protocol::percent_type quorum = 0u;
    quorum_type committee_quorum = none_quorum;

    void validate() const;

    protocol::percent_type get_required_quorum(committee_i& committee_service) const;
};

struct development_committee_add_member_operation
    : public proposal_base_operation<development_committee_add_member_operation, development_committee_i>
{
    account_name_type account_name;

    void validate() const;

    protocol::percent_type get_required_quorum(committee_i& committee_service) const;
};

struct development_committee_exclude_member_operation
    : public proposal_base_operation<development_committee_exclude_member_operation, development_committee_i>
{
    account_name_type account_name;

    void validate() const;

    protocol::percent_type get_required_quorum(committee_i& committee_service) const;
};

struct development_committee_change_quorum_operation
    : public proposal_base_operation<development_committee_change_quorum_operation, development_committee_i>
{
    protocol::percent_type quorum = 0u;
    quorum_type committee_quorum = none_quorum;

    void validate() const;

    protocol::percent_type get_required_quorum(committee_i& committee_service) const;
};

struct development_committee_withdraw_vesting_operation
    : public proposal_base_operation<development_committee_withdraw_vesting_operation, development_committee_i>
{
    asset vesting_shares = asset(0, SP_SYMBOL);

    void validate() const;

    protocol::percent_type get_required_quorum(committee_i& committee_service) const;
};

struct development_committee_transfer_operation
    : public proposal_base_operation<development_committee_withdraw_vesting_operation, development_committee_i>
{
    account_name_type to_account;
    asset amount = asset(0, SCORUM_SYMBOL);

    void validate() const;

    protocol::percent_type get_required_quorum(committee_i& committee_service) const;
};

using proposal_operation = fc::static_variant<registration_committee_add_member_operation,
                                              registration_committee_exclude_member_operation,
                                              registration_committee_change_quorum_operation,
                                              development_committee_add_member_operation,
                                              development_committee_exclude_member_operation,
                                              development_committee_change_quorum_operation,
                                              development_committee_withdraw_vesting_operation,
                                              development_committee_transfer_operation>;

struct to_committee_operation
{
    template <typename T> const typename T::committee_operation_type& cast(const proposal_operation& op)
    {
        const typename T::committee_operation_type& operation = op.get<T>();
        return operation;
    }
};

struct operation_get_required_quorum_visitor
{
    typedef scorum::protocol::percent_type result_type;

    operation_get_required_quorum_visitor(committee_i& committee_service)
        : _committee_service(committee_service)
    {
    }

    template <typename T> protocol::percent_type operator()(const T& v) const
    {
        return v.get_required_quorum(_committee_service);
    }

private:
    committee_i& _committee_service;
};

void operation_validate(const proposal_operation& op);
protocol::percent_type operation_get_required_quorum(committee_i& committee_service, const proposal_operation& op);

} // namespace protocol
} // namespace scorum

// clang-format off
FC_REFLECT_ENUM(scorum::protocol::quorum_type,
                (none_quorum)
                (add_member_quorum)
                (exclude_member_quorum)
                (base_quorum)
                (transfer_quorum))
// clang-format on

FC_REFLECT(scorum::protocol::registration_committee_add_member_operation, (account_name))
FC_REFLECT(scorum::protocol::registration_committee_exclude_member_operation, (account_name))
FC_REFLECT(scorum::protocol::registration_committee_change_quorum_operation, (quorum)(committee_quorum))

FC_REFLECT(scorum::protocol::development_committee_add_member_operation, (account_name))
FC_REFLECT(scorum::protocol::development_committee_exclude_member_operation, (account_name))
FC_REFLECT(scorum::protocol::development_committee_change_quorum_operation, (quorum)(committee_quorum))

FC_REFLECT(scorum::protocol::development_committee_withdraw_vesting_operation, (vesting_shares))
FC_REFLECT(scorum::protocol::development_committee_transfer_operation, (amount)(to_account))

DECLARE_OPERATION_SERIALIZATOR(scorum::protocol::proposal_operation)
FC_REFLECT_TYPENAME(scorum::protocol::proposal_operation)
