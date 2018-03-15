#pragma once

#include <scorum/chain/services/service_base.hpp>
#include <scorum/chain/schema/withdraw_scorumpower_objects.hpp>

namespace scorum {
namespace chain {

class withdraw_scorumpower_route_object;
class account_object;
class dev_committee_object;

struct withdraw_scorumpower_route_service_i : public base_service_i<withdraw_scorumpower_route_object>
{
    virtual bool is_exists(const account_id_type& from, const account_id_type& to) const = 0;

    virtual bool is_exists(const account_id_type& from, const dev_committee_id_type& to) const = 0;

    virtual bool is_exists(const dev_committee_id_type& from, const dev_committee_id_type& to) const = 0;

    virtual bool is_exists(const dev_committee_id_type& from, const account_id_type& to) const = 0;

    virtual const withdraw_scorumpower_route_object& get(const account_id_type& from,
                                                         const account_id_type& to) const = 0;

    virtual const withdraw_scorumpower_route_object& get(const account_id_type& from,
                                                         const dev_committee_id_type& to) const = 0;

    virtual const withdraw_scorumpower_route_object& get(const dev_committee_id_type& from,
                                                         const dev_committee_id_type& to) const = 0;

    virtual const withdraw_scorumpower_route_object& get(const dev_committee_id_type& from,
                                                         const account_id_type& to) const = 0;

    using withdraw_scorumpower_route_refs_type
        = std::vector<std::reference_wrapper<const withdraw_scorumpower_route_object>>;

    virtual withdraw_scorumpower_route_refs_type get_all(const withdrawable_id_type& from) const = 0;

    virtual uint16_t total_percent(const account_id_type& from) const = 0;

    virtual uint16_t total_percent(const dev_committee_id_type& from) const = 0;
};

class dbs_withdraw_scorumpower_route_impl;

class dbs_withdraw_scorumpower_route : public dbs_service_base<withdraw_scorumpower_route_service_i>
{
    friend class dbservice_dbs_factory;

protected:
    explicit dbs_withdraw_scorumpower_route(database& db);

public:
    ~dbs_withdraw_scorumpower_route();

    virtual bool is_exists(const account_id_type& from, const account_id_type& to) const override;

    virtual bool is_exists(const account_id_type& from, const dev_committee_id_type& to) const override;

    virtual bool is_exists(const dev_committee_id_type& from, const dev_committee_id_type& to) const override;

    virtual bool is_exists(const dev_committee_id_type& from, const account_id_type& to) const override;

    virtual const withdraw_scorumpower_route_object& get(const account_id_type& from,
                                                         const account_id_type& to) const override;

    virtual const withdraw_scorumpower_route_object& get(const account_id_type& from,
                                                         const dev_committee_id_type& to) const override;

    virtual const withdraw_scorumpower_route_object& get(const dev_committee_id_type& from,
                                                         const dev_committee_id_type& to) const override;

    virtual const withdraw_scorumpower_route_object& get(const dev_committee_id_type& from,
                                                         const account_id_type& to) const override;

    virtual withdraw_scorumpower_route_refs_type get_all(const withdrawable_id_type& from) const override;

    virtual uint16_t total_percent(const account_id_type& from) const override;

    virtual uint16_t total_percent(const dev_committee_id_type& from) const override;

private:
    std::unique_ptr<dbs_withdraw_scorumpower_route_impl> _impl;
};
} // namespace chain
} // namespace scorum
