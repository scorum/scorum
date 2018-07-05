#pragma once

#include <scorum/protocol/proposal_operations.hpp>
#include <scorum/chain/data_service_factory.hpp>
#include <scorum/chain/services/registration_committee.hpp>
#include <scorum/chain/services/development_committee.hpp>

namespace scorum {
namespace chain {

struct data_service_factory_i;

protocol::committee get_committee(data_service_factory_i& services,
                                  const protocol::proposal_committee_operation<protocol::registration_committee_i>&);

protocol::committee get_committee(data_service_factory_i& services,
                                  const protocol::proposal_committee_operation<protocol::development_committee_i>&);

struct get_operation_committee_visitor
{
    typedef protocol::committee result_type;

    get_operation_committee_visitor(data_service_factory_i& services)
        : _services(services)
    {
    }

    template <typename T> result_type operator()(const T& v) const
    {
        return get_committee(_services, v);
    }

private:
    data_service_factory_i& _services;
};

} // namespace scorum
} // namespace chain
