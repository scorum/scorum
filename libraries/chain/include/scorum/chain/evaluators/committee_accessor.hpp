#pragma once

#include <scorum/protocol/proposal_operations.hpp>
#include <scorum/chain/data_service_factory.hpp>
#include <scorum/chain/services/registration_committee.hpp>

namespace scorum {
namespace chain {

class data_service_factory_i;

struct committee_accessor
{
    explicit committee_accessor(data_service_factory_i& services)
        : _services(services)
    {
    }

    //    protocol::committee&
    //    committee_accessor::get_committee(const
    //    protocol::proposal_committee_operation<protocol::registration_committee>&) const
    //    {
    //        return _services.registration_committee_service();
    //    }
    protocol::committee& get_committee(const protocol::proposal_operation& op)
    {
        using namespace scorum::protocol;

        if (op.which() == proposal_operation::tag<registration_committee_add_member_operation>::value)
        {
            const auto& o = to_committee_operation().cast<registration_committee_add_member_operation>(op);
            return get_committee(o);
        }
        else if (op.which() == proposal_operation::tag<registration_committee_exclude_member_operation>::value)
        {
            const auto& o = to_committee_operation().cast<registration_committee_exclude_member_operation>(op);
            return get_committee(o);
        }
        else if (op.which() == proposal_operation::tag<registration_committee_change_quorum_operation>::value)
        {
            const auto& o = to_committee_operation().cast<registration_committee_change_quorum_operation>(op);
            return get_committee(o);
        }
        else
        {
            FC_THROW_EXCEPTION(fc::assert_exception, "Unknow operation.");
        }
    }

    protocol::committee& get_committee(const protocol::proposal_committee_operation<protocol::registration_committee>&)
    {
        return _services.registration_committee_service();
    }

private:
    data_service_factory_i& _services;
};

// template <>
// protocol::committee&
// committee_accessor::get_committee(const protocol::proposal_committee_operation<protocol::registration_committee>&)
// const
//{
//    return _services.registration_committee_service();
//}

} // namespace scorum
} // namespace chain
