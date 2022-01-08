#include <scorum/chain/evaluators/game_round_result_evaluator.hpp>

#include <scorum/chain/schema/account_objects.hpp>
#include <scorum/chain/schema/nft_object.hpp>

#include <scorum/chain/data_service_factory.hpp>
#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/hardfork_property.hpp>
#include <scorum/chain/dba/db_accessor.hpp>

namespace scorum {
namespace chain {

game_round_result_evaluator::game_round_result_evaluator(data_service_factory_i& services,
                                           dba::db_accessor<account_object>& account_dba,
                                           dba::db_accessor<game_round_object>& game_round_dba)
    : evaluator_impl<data_service_factory_i, game_round_result_evaluator>(services)
    , _account_dba(account_dba)
    , _game_round_dba(game_round_dba)
    , _hardfork_service(services.hardfork_property_service())
{
}

void game_round_result_evaluator::do_apply(const operation_type& op)
{
    FC_ASSERT(_hardfork_service.has_hardfork(SCORUM_HARDFORK_0_5), "Hardfork #${hf} is required", ("hf", SCORUM_HARDFORK_0_5));

    FC_ASSERT(_game_round_dba.is_exists_by<by_uuid>(op.uuid), R"(Round "${uuid}" must exist.)", ("uuid", op.uuid));
    FC_ASSERT(_account_dba.is_exists_by<by_name>(op.owner), R"(Account "${owner}" must exist.)", ("owner", op.owner));

    auto& round = _game_round_dba.get_by<by_uuid>(op.uuid);

    FC_ASSERT(round.owner == op.owner, "${round.owner} != ${operation.owner}", ("round.owner", round.owner)("operation.owner", op.owner));

    _game_round_dba.update(round, [&](auto& obj){
        obj.result = op.result;
        fc::from_string(obj.vrf, op.vrf);
        fc::from_string(obj.proof, op.proof);
    });
}

} // namespace chain
} // namespace scorum
