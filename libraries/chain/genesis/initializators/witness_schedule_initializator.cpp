#include <scorum/chain/genesis/initializators/witness_schedule_initializator.hpp>
#include <scorum/chain/data_service_factory.hpp>

#include <scorum/chain/services/witness.hpp>

#include <scorum/chain/schema/witness_objects.hpp>

#include <scorum/chain/genesis/genesis_state.hpp>

namespace scorum {
namespace chain {
namespace genesis {

void witness_schedule_initializator_impl::apply(data_service_factory_i& services,
                                                const genesis_state_type& genesis_state)
{
    witness_service_i& witness_service = services.witness_service();

    FC_ASSERT(!witness_service.is_exists());

    const std::vector<genesis_state_type::witness_type>& witness_candidates = genesis_state.witness_candidates;

    witness_service.create_witness_schedule([&](witness_schedule_object& wso) {
        for (size_t i = 0; i < wso.current_shuffled_witnesses.size() && i < witness_candidates.size(); ++i)
        {
            FC_ASSERT(witness_service.is_exists(witness_candidates[i].name));

            wso.current_shuffled_witnesses[i] = witness_candidates[i].name;
        }
    });
}
}
}
}
