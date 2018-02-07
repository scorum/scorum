#pragma once

#include <scorum/chain/genesis/genesis_state.hpp>
#include <scorum/chain/genesis/initializators/initializators.hpp>

namespace scorum {
namespace chain {

class database;

class db_genesis : public genesis::initializators_registry
{
public:
    db_genesis(db_genesis const&) = delete;
    db_genesis& operator=(db_genesis const&) = delete;

    db_genesis(database& db, const genesis_state_type& genesis_state);

private:
    void init_accounts();
    void init_founders();
    void init_witnesses();
    void init_witness_schedule();
    void init_global_property_object();
    void init_rewards();
    void init_registration_objects();

    database& _db;
    genesis_state_type _genesis_state;
};

} // namespace chain
} // namespace scorum
