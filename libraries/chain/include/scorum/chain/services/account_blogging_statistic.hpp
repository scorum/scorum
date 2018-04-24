#pragma once

#include <scorum/chain/services/service_base.hpp>
#include <scorum/chain/schema/account_objects.hpp>

namespace scorum {
namespace chain {

struct account_blogging_statistic_service_i : public base_service_i<account_blogging_statistic_object>
{
    virtual const account_blogging_statistic_object& obtain(const account_id_type& account_id) = 0;

    virtual const account_blogging_statistic_object* find(const account_id_type& account_id) const = 0;

    virtual void add_post(const account_blogging_statistic_object& stat) = 0;

    virtual void add_comment(const account_blogging_statistic_object& stat) = 0;

    virtual void add_vote(const account_blogging_statistic_object& stat) = 0;

    virtual void increase_posting_rewards(const account_blogging_statistic_object& stat, const asset& amount) = 0;

    virtual void increase_curation_rewards(const account_blogging_statistic_object& stat, const asset& amount) = 0;
};

class dbs_account_blogging_statistic : public dbs_service_base<account_blogging_statistic_service_i>
{
    friend class dbservice_dbs_factory;

protected:
    explicit dbs_account_blogging_statistic(database& db);

public:
    const account_blogging_statistic_object& obtain(const account_id_type& account_id) override;

    const account_blogging_statistic_object* find(const account_id_type& account_id) const override;

    void add_post(const account_blogging_statistic_object& stat) override;

    void add_comment(const account_blogging_statistic_object& stat) override;

    void add_vote(const account_blogging_statistic_object& stat) override;

    void increase_posting_rewards(const account_blogging_statistic_object& stat, const asset& amount) override;

    void increase_curation_rewards(const account_blogging_statistic_object& stat, const asset& amount) override;
};

} // namespace chain
} // namespace scorum
