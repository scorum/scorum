#include <scorum/chain/evaluators/committee_accessor.hpp>

namespace scorum {
namespace chain {

protocol::committee_i& get_committee(data_service_factory_i& services,
                                     const protocol::proposal_committee_operation<protocol::registration_committee_i>&)
{
    return services.registration_committee_service();
}

protocol::committee_i& get_committee(data_service_factory_i& services,
                                     const protocol::proposal_committee_operation<protocol::development_committee_i>&)
{
    return services.development_committee_service();
}
}
}
