#pragma once
#include <scorum/chain/schema/bet_objects.hpp>
#include <scorum/chain/services/service_base.hpp>

namespace scorum {
namespace chain {

struct pending_bet_service_i : public base_service_i<pending_bet_object>
{
    using pending_bet_call_type = std::function<bool(const base_service_i::object_type&)>;
    using base_service_i<pending_bet_object>::is_exists;

    virtual bool is_exists(const uuid_type& uuid) const = 0;
    virtual const pending_bet_object& get_pending_bet(const uuid_type&) const = 0;
};

class dbs_pending_bet : public dbs_service_base<pending_bet_service_i>
{
    friend class dbservice_dbs_factory;

protected:
    explicit dbs_pending_bet(database& db);

public:
    using base_service_i<pending_bet_object>::is_exists;

    bool is_exists(const uuid_type& uuid) const override;
    const pending_bet_object& get_pending_bet(const uuid_type&) const override;
};
}
}
