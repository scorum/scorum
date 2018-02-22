#pragma once

#include <scorum/protocol/types.hpp>
#include <scorum/protocol/operation_util.hpp>
#include <fc/static_variant.hpp>

namespace scorum {
namespace protocol {

enum quorum_type
{
    none_quorum,
    add_member_quorum,
    exclude_member_quorum,
    base_quorum
};

struct committee
{
    virtual void add_member(const account_name_type&) = 0;
    virtual void exclude_member(const account_name_type&) = 0;
    virtual void change_add_member_quorum(const uint32_t quorum) = 0;
    virtual void change_exclude_member_quorum(const uint32_t quorum) = 0;
    virtual void change_quorum(const uint32_t quorum) = 0;
};

struct registration_committee : public committee
{
};

struct development_committee : public committee
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
    : public proposal_base_operation<registration_committee_add_member_operation, registration_committee>
{
    account_name_type account_name;
};

struct registration_committee_exclude_member_operation
    : public proposal_base_operation<registration_committee_exclude_member_operation, registration_committee>
{
    account_name_type account_name;
};

struct registration_committee_change_quorum_operation
    : public proposal_base_operation<registration_committee_change_quorum_operation, registration_committee>
{
    protocol::percent_type quorum = 0u;
    quorum_type committee_quorum = none_quorum;
};

// clang-format off
using proposal_operation = fc::static_variant<registration_committee_add_member_operation,
                                              registration_committee_exclude_member_operation,
                                              registration_committee_change_quorum_operation>;
// clang-format on

} // namespace protocol
} // namespace scorum

FC_REFLECT(scorum::protocol::registration_committee_add_member_operation, (account_name))
FC_REFLECT(scorum::protocol::registration_committee_exclude_member_operation, (account_name))
FC_REFLECT(scorum::protocol::registration_committee_change_quorum_operation, (quorum))

DECLARE_OPERATION_SERIALIZATOR(scorum::protocol::proposal_operation)
FC_REFLECT_TYPENAME(scorum::protocol::proposal_operation)
