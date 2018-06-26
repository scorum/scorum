#include <scorum/chain/evaluators/committee_accessor.hpp>

namespace scorum {
namespace chain {

protocol::committee get_committee(data_service_factory_i& services,
                                  const protocol::proposal_committee_operation<protocol::registration_committee_i>&)
{
    return scorum::utils::make_ref(static_cast<registration_committee_i&>(services.registration_committee_service()));
}

protocol::committee get_committee(data_service_factory_i& services,
                                  const protocol::proposal_committee_operation<protocol::development_committee_i>&)
{
    return scorum::utils::make_ref(static_cast<development_committee_i&>(services.development_committee_service()));
}
}
}
