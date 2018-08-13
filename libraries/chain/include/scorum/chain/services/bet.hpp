#pragma once
#include <scorum/chain/schema/bet_objects.hpp>
#include <scorum/chain/services/service_base.hpp>

namespace scorum {
namespace chain {

struct bet_service_i : public base_service_i<bet_object>
{
    virtual const bet_object& get_bet(const bet_id_type&) const = 0;
};

class dbs_bet : public dbs_service_base<bet_service_i>
{
    friend class dbservice_dbs_factory;

protected:
    explicit dbs_bet(database& db);

public:
    virtual const bet_object& get_bet(const bet_id_type&) const override;
};
}
}
