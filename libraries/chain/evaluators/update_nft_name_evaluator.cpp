#include <scorum/chain/evaluators/update_nft_name_evaluator.hpp>

#include <scorum/chain/schema/account_objects.hpp>
#include <scorum/chain/schema/nft_object.hpp>

#include <scorum/chain/data_service_factory.hpp>
#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/hardfork_property.hpp>
#include <scorum/chain/dba/db_accessor.hpp>

namespace scorum {
namespace chain {

update_nft_name_evaluator::update_nft_name_evaluator(data_service_factory_i& services,
                                                     dba::db_accessor<account_object>& account_dba,
                                                     dba::db_accessor<nft_object>& nft_dba)
    : evaluator_impl<data_service_factory_i, update_nft_name_evaluator>(services)
    , _account_dba(account_dba)
    , _nft_dba(nft_dba)
    , _hardfork_service(services.hardfork_property_service())
{
}

void update_nft_name_evaluator::do_apply(const operation_type& op)
{
    FC_ASSERT(_hardfork_service.has_hardfork(SCORUM_HARDFORK_0_6), "Hardfork #6 is required");
    FC_ASSERT(_nft_dba.is_exists_by<by_uuid>(op.uuid), R"(NFT with uuid "${uuid}" must exist.)", ("uuid", op.uuid));
    FC_ASSERT(_account_dba.is_exists_by<by_name>(op.moderator), R"(Account "${moderator}" must exist.)",
              ("moderator", op.moderator));

    auto& nft = _nft_dba.get_by<by_uuid>(op.uuid);
    _nft_dba.update(nft, [&](auto& nft) {
        nft.name = op.name;
    });
}

} // namespace chain
} // namespace scorum
