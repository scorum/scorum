#pragma once

#include <scorum/chain/services/dbs_base.hpp>

namespace scorum {
namespace chain {

class withdraw_vesting_route_object;
class account_object;
class dev_committee_object;

struct withdraw_vesting_route_service_i
{
    virtual bool is_exists(account_id_type from, account_id_type to) const = 0;

    virtual bool is_exists(account_id_type from, dev_committee_id_type to) const = 0;

    virtual const withdraw_vesting_route_object& get(account_id_type from, account_id_type to) const = 0;

    virtual const withdraw_vesting_route_object& get(account_id_type from, dev_committee_id_type to) const = 0;

    virtual void remove(const withdraw_vesting_route_object& obj) = 0;

    using modifier_type = std::function<void(withdraw_vesting_route_object&)>;

    virtual const withdraw_vesting_route_object& create(const modifier_type&) = 0;

    virtual void update(const withdraw_vesting_route_object& obj, const modifier_type&) = 0;

    virtual uint16_t total_percent(account_id_type from) const = 0;
};

class dbs_withdraw_vesting_route : public dbs_base, public withdraw_vesting_route_service_i
{
    friend class dbservice_dbs_factory;

protected:
    explicit dbs_withdraw_vesting_route(database& db);

public:
    virtual bool is_exists(account_id_type from, account_id_type to) const override;

    virtual bool is_exists(account_id_type from, dev_committee_id_type to) const override;

    virtual const withdraw_vesting_route_object& get(account_id_type from, account_id_type to) const override;

    virtual const withdraw_vesting_route_object& get(account_id_type from, dev_committee_id_type to) const override;

    virtual void remove(const withdraw_vesting_route_object& obj) override;

    virtual const withdraw_vesting_route_object& create(const modifier_type&) override;

    virtual void update(const withdraw_vesting_route_object& obj, const modifier_type&) override;

    virtual uint16_t total_percent(account_id_type from) const override;
};

namespace withdraw_route_service {

uint128_t get_to_id(const account_id_type&);
uint128_t get_to_id(const account_object&);
uint128_t get_to_id(const dev_committee_id_type&);
uint128_t get_to_id(const dev_committee_object&);

bool is_to_account(const withdraw_vesting_route_object&);
bool is_to_dev_committee(const withdraw_vesting_route_object&);
}
} // namespace chain
} // namespace scorum
