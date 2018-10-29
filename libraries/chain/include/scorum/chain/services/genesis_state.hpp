#pragma once

#include <scorum/chain/services/dbs_base.hpp>

namespace scorum {
namespace chain {

struct genesis_state_service_i
{
    virtual const fc::time_point_sec& get_lock_withdraw_sp_until_timestamp() const = 0;
};

class dbs_genesis_state : public dbs_base, public genesis_state_service_i
{
    friend class dbservice_dbs_factory;

protected:
    explicit dbs_genesis_state(database& db);

public:
    const fc::time_point_sec& get_lock_withdraw_sp_until_timestamp() const override;

private:
    // TODO: tech dept: remove this dependency if possible
    database& _db;
};
} // namespace chain
} // namespace scorum
