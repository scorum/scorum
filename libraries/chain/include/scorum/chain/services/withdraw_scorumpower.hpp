#pragma once

#include <scorum/chain/services/service_base.hpp>
#include <scorum/chain/schema/withdraw_scorumpower_objects.hpp>

namespace scorum {
namespace chain {

struct withdraw_scorumpower_service_i : public base_service_i<withdraw_scorumpower_object>
{
    using base_service_i<withdraw_scorumpower_object>::get;
    using base_service_i<withdraw_scorumpower_object>::is_exists;

    virtual bool is_exists(const account_id_type& from) const = 0;

    virtual bool is_exists(const dev_committee_id_type& from) const = 0;

    virtual const withdraw_scorumpower_object& get(const account_id_type& from) const = 0;

    virtual const withdraw_scorumpower_object& get(const dev_committee_id_type& from) const = 0;

    using withdraw_scorumpower_refs_type = std::vector<std::reference_wrapper<const withdraw_scorumpower_object>>;

    virtual withdraw_scorumpower_refs_type get_until(const time_point_sec& until) const = 0;

    virtual asset get_withdraw_rest(const account_id_type& from) const = 0;
};

class dbs_withdraw_scorumpower : public dbs_service_base<withdraw_scorumpower_service_i>
{
    friend class dbservice_dbs_factory;

protected:
    explicit dbs_withdraw_scorumpower(database& db);

public:
    using base_service_i<withdraw_scorumpower_object>::get;
    using base_service_i<withdraw_scorumpower_object>::is_exists;

    ~dbs_withdraw_scorumpower();

    virtual bool is_exists(const account_id_type& from) const override;

    virtual bool is_exists(const dev_committee_id_type& from) const override;

    virtual const withdraw_scorumpower_object& get(const account_id_type& from) const override;

    virtual const withdraw_scorumpower_object& get(const dev_committee_id_type& from) const override;

    virtual withdraw_scorumpower_refs_type get_until(const time_point_sec& until) const override;

    virtual asset get_withdraw_rest(const account_id_type& from) const override;
};

} // namespace chain
} // namespace scorum
