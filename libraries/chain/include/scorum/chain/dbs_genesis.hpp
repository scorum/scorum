#pragma once

#include <scorum/chain/dbs_base_impl.hpp>

namespace scorum {
namespace chain {

class database;
class genesis_state_type;

class dbs_genesis : public dbs_base
{
    friend class dbservice_dbs_factory;

protected:
    explicit dbs_genesis(database& db);

public:
    void init_accounts(const genesis_state_type& genesis_state);
    void init_witnesses(const genesis_state_type& genesis_state);
    void init_witness_schedule(const genesis_state_type& genesis_state);
    void init_global_property_object(const genesis_state_type& genesis_state);
    void init_rewards(const genesis_state_type& genesis_state);
};

} // namespace chain
} // namespace scorum
