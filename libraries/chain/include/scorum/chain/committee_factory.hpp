#pragma once

#include <scorum/protocol/proposal_operations.hpp>

#include <scorum/chain/services/registration_committee.hpp>

#include <scorum/chain/data_service_factory.hpp>
#include <fc/exception/exception.hpp>

namespace scorum {
namespace chain {

struct committee_factory
{
    committee_factory(data_service_factory_i& factory)
        : _factory(factory)
    {
    }

    template <typename OperationType> protocol::committee& obtain_committee(const OperationType&)
    {
        FC_THROW_EXCEPTION(fc::assert_exception, "Operation not implemented.");
    }

    template <typename OperationType> protocol::percent_type get_quorum(const OperationType&)
    {
        FC_THROW_EXCEPTION(fc::assert_exception, "Operation not implemented.");
    }

    scorum::chain::data_service_factory_i& _factory;
};

template <>
protocol::committee&
committee_factory::obtain_committee(const protocol::proposal_committee_operation<protocol::registration_committee>&)
{
    return _factory.registration_committee_service();
}

template <> protocol::percent_type committee_factory::get_quorum(const registration_committee_add_member_operation&)
{
    return SCORUM_COMMITTEE_QUORUM_PERCENT;
}

} // namespace chain
} // namespace scorum
