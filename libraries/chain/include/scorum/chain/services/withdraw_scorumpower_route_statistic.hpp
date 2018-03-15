#pragma once

#include <scorum/chain/services/service_base.hpp>
#include <scorum/chain/schema/withdraw_scorumpower_objects.hpp>

namespace scorum {
namespace chain {

struct withdraw_scorumpower_route_statistic_service_i
    : public base_service_i<withdraw_scorumpower_route_statistic_object>
{
    virtual bool is_exists(const account_id_type& from) const = 0;

    virtual bool is_exists(const dev_committee_id_type& from) const = 0;

    virtual const withdraw_scorumpower_route_statistic_object& get(const account_id_type& from) const = 0;

    virtual const withdraw_scorumpower_route_statistic_object& get(const dev_committee_id_type& from) const = 0;
};

class dbs_withdraw_scorumpower_route_statistic : public dbs_service_base<withdraw_scorumpower_route_statistic_service_i>
{
    friend class dbservice_dbs_factory;

protected:
    explicit dbs_withdraw_scorumpower_route_statistic(database& db);

public:
    ~dbs_withdraw_scorumpower_route_statistic();

    virtual bool is_exists(const account_id_type& from) const override;

    virtual bool is_exists(const dev_committee_id_type& from) const override;

    virtual const withdraw_scorumpower_route_statistic_object& get(const account_id_type& from) const override;

    virtual const withdraw_scorumpower_route_statistic_object& get(const dev_committee_id_type& from) const override;
};

} // namespace chain
} // namespace scorum
