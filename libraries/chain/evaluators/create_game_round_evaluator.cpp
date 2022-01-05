#include <scorum/chain/evaluators/create_game_round_evaluator.hpp>

#include <scorum/chain/schema/account_objects.hpp>
#include <scorum/chain/schema/nft_object.hpp>

#include <scorum/chain/data_service_factory.hpp>
#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/hardfork_property.hpp>
#include <scorum/chain/dba/db_accessor.hpp>

namespace scorum {
namespace chain {
create_game_round_evaluator::create_game_round_evaluator(data_service_factory_i& services,
                                           dba::db_accessor<account_object>& account_dba,
                                           dba::db_accessor<game_round_object>& game_round_dba)
    : evaluator_impl<data_service_factory_i, create_game_round_evaluator>(services)
    , _account_dba(account_dba)
    , _game_round_dba(game_round_dba)
    , _hardfork_service(services.hardfork_property_service())
{
}

void create_game_round_evaluator::do_apply(const operation_type& op)
{
    FC_ASSERT(_hardfork_service.has_hardfork(SCORUM_HARDFORK_0_5), "Hardfork #5 is required");

    FC_ASSERT(!_game_round_dba.is_exists_by<by_uuid>(op.uuid), R"(Round with uuid "${uuid}" already exist.)", ("uuid", op.uuid));
    FC_ASSERT(_account_dba.is_exists_by<by_name>(op.owner), R"(Account "${owner}" must exist.)", ("owner", op.owner));

    _game_round_dba.create([&](auto& obj) {
        obj.uuid = op.uuid;
        obj.owner = op.owner;

        fc::from_string(obj.seed, op.seed);
        fc::from_string(obj.verification_key, op.verification_key);
    });
}

} // namespace chain
} // namespace scorum
