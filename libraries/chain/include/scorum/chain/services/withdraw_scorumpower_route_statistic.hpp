#pragma once

#include <scorum/chain/services/dbs_base.hpp>

#include <memory>

namespace scorum {
namespace chain {

class withdraw_scorumpower_route_statistic_object;

struct withdraw_scorumpower_route_statistic_service_i
{
    virtual bool is_exists(const account_id_type& from) const = 0;

    virtual bool is_exists(const dev_committee_id_type& from) const = 0;

    virtual const withdraw_scorumpower_route_statistic_object& get(const account_id_type& from) const = 0;

    virtual const withdraw_scorumpower_route_statistic_object& get(const dev_committee_id_type& from) const = 0;

    using modifier_type = std::function<void(withdraw_scorumpower_route_statistic_object&)>;

    virtual const withdraw_scorumpower_route_statistic_object& create(const modifier_type&) = 0;

    virtual void update(const withdraw_scorumpower_route_statistic_object& obj, const modifier_type&) = 0;

    virtual void remove(const withdraw_scorumpower_route_statistic_object& obj) = 0;
};

class dbs_withdraw_scorumpower_route_statistic_impl;

class dbs_withdraw_scorumpower_route_statistic : public dbs_base, public withdraw_scorumpower_route_statistic_service_i
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

    virtual const withdraw_scorumpower_route_statistic_object& create(const modifier_type&) override;

    virtual void update(const withdraw_scorumpower_route_statistic_object& obj, const modifier_type&) override;

    virtual void remove(const withdraw_scorumpower_route_statistic_object& obj) override;

private:
    std::unique_ptr<dbs_withdraw_scorumpower_route_statistic_impl> _impl;
};

} // namespace chain
} // namespace scorum
