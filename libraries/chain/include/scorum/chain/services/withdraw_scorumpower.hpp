#pragma once

#include <scorum/chain/services/dbs_base.hpp>

#include <memory>
#include <vector>
#include <functional>

namespace scorum {
namespace chain {

class withdraw_scorumpower_object;

struct withdraw_scorumpower_service_i
{
    virtual bool is_exists(const account_id_type& from) const = 0;

    virtual bool is_exists(const dev_committee_id_type& from) const = 0;

    virtual const withdraw_scorumpower_object& get(const account_id_type& from) const = 0;

    virtual const withdraw_scorumpower_object& get(const dev_committee_id_type& from) const = 0;

    using withdraw_scorumpower_refs_type = std::vector<std::reference_wrapper<const withdraw_scorumpower_object>>;

    virtual withdraw_scorumpower_refs_type get_until(const time_point_sec& until) const = 0;

    virtual asset get_withdraw_rest(const account_id_type& from) const = 0;

    using modifier_type = std::function<void(withdraw_scorumpower_object&)>;

    virtual const withdraw_scorumpower_object& create(const modifier_type&) = 0;

    virtual void update(const withdraw_scorumpower_object& obj, const modifier_type&) = 0;

    virtual void remove(const withdraw_scorumpower_object& obj) = 0;
};

class dbs_withdraw_scorumpower_impl;

class dbs_withdraw_scorumpower : public dbs_base, public withdraw_scorumpower_service_i
{
    friend class dbservice_dbs_factory;

protected:
    explicit dbs_withdraw_scorumpower(database& db);

public:
    ~dbs_withdraw_scorumpower();

    virtual bool is_exists(const account_id_type& from) const override;

    virtual bool is_exists(const dev_committee_id_type& from) const override;

    virtual const withdraw_scorumpower_object& get(const account_id_type& from) const override;

    virtual const withdraw_scorumpower_object& get(const dev_committee_id_type& from) const override;

    virtual withdraw_scorumpower_refs_type get_until(const time_point_sec& until) const override;

    virtual asset get_withdraw_rest(const account_id_type& from) const override;

    virtual const withdraw_scorumpower_object& create(const modifier_type&) override;

    virtual void update(const withdraw_scorumpower_object& obj, const modifier_type&) override;

    virtual void remove(const withdraw_scorumpower_object& obj) override;

private:
    std::unique_ptr<dbs_withdraw_scorumpower_impl> _impl;
};

} // namespace chain
} // namespace scorum
