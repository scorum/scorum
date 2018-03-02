#pragma once

#include <scorum/protocol/proposal_operations.hpp>

#include <scorum/chain/services/dbs_base.hpp>
#include <vector>
#include <set>
#include <functional>

namespace scorum {
namespace chain {

struct development_committee_service_i : public development_committee_i
{
};

struct dbs_development_committee : public dbs_base, public development_committee_service_i
{
    void add_member(const account_name_type&) override;
    void exclude_member(const account_name_type&) override;

    void change_add_member_quorum(const percent_type quorum) override;
    void change_exclude_member_quorum(const percent_type quorum) override;
    void change_base_quorum(const percent_type quorum) override;

    percent_type get_add_member_quorum() override;
    percent_type get_exclude_member_quorum() override;
    percent_type get_base_quorum() override;

    bool is_exists(const account_name_type&) const override;

    virtual size_t get_members_count() const override;

private:
    friend class dbservice_dbs_factory;

protected:
    explicit dbs_development_committee(database& db);
};

} // namespace chain
} // namespace scorum
