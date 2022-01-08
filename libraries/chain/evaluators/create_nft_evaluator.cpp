#include <scorum/chain/evaluators/create_nft_evaluator.hpp>

#include <scorum/chain/schema/account_objects.hpp>
#include <scorum/chain/schema/nft_object.hpp>

#include <scorum/chain/data_service_factory.hpp>
#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/hardfork_property.hpp>
#include <scorum/chain/dba/db_accessor.hpp>

namespace scorum {
namespace chain {
create_nft_evaluator::create_nft_evaluator(data_service_factory_i& services,
                                           dba::db_accessor<account_object>& account_dba,
                                           dba::db_accessor<nft_object>& nft_dba)
    : evaluator_impl<data_service_factory_i, create_nft_evaluator>(services)
    , _account_dba(account_dba)
    , _nft_dba(nft_dba)
    , _dprop_service(services.dynamic_global_property_service())
    , _hardfork_service(services.hardfork_property_service())
{
}

void create_nft_evaluator::do_apply(const operation_type& op)
{
    FC_ASSERT(_hardfork_service.has_hardfork(SCORUM_HARDFORK_0_5), "Hardfork #5 is required");

    FC_ASSERT(!_nft_dba.is_exists_by<by_name>(op.name), R"(NFT with name "${name}" already exists.)",
              ("name", op.name));
    FC_ASSERT(!_nft_dba.is_exists_by<by_uuid>(op.uuid), R"(NFT with uuid "${uuid}" already exists.)",
              ("uuid", op.uuid));
    FC_ASSERT(_account_dba.is_exists_by<by_name>(op.owner), R"(Account "${owner}" must exist.)", ("owner", op.owner));

    auto& account = _account_dba.get_by<by_name>(op.owner);
    const auto available_power = account.scorumpower - account.nft_spend_scorumpower;

    share_type requested_sp = op.power * pow(10, SCORUM_CURRENCY_PRECISION);

    FC_ASSERT(available_power.amount >= requested_sp,
              R"(Account available power "${available_power}" is less than requested "${requested_sp}".)",
              ("available_power", available_power.amount)("requested_sp", requested_sp));

    _account_dba.update(account, [&](auto& obj) {
        obj.nft_spend_scorumpower.amount += requested_sp;
    });

    _nft_dba.create([&](auto& obj) {
        obj.uuid = op.uuid;
        obj.name = op.name;
        obj.owner = op.owner;
        obj.power += op.power;
        obj.created = _dprop_service.head_block_time();

#ifndef IS_LOW_MEM
        fc::from_string(obj.json_metadata, op.json_metadata);
#endif
    });
}

} // namespace chain
} // namespace scorum
