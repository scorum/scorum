#pragma once

#include <vector>
#include <set>
#include <functional>

#include <scorum/protocol/proposal_operations.hpp>

#include <scorum/chain/services/dbs_base.hpp>

namespace scorum {
namespace chain {

class registration_committee_member_object;

struct registration_committee_service_i : public scorum::protocol::registration_committee_i
{
    using committee_member_object_cref_type = std::reference_wrapper<const registration_committee_member_object>;
    using committee_members_cref_type = std::vector<committee_member_object_cref_type>;

    virtual committee_members_cref_type get_committee() const = 0;

    virtual const registration_committee_member_object& get_member(const account_name_type&) const = 0;

    virtual committee_members_cref_type create_committee(const std::vector<account_name_type>& accounts) = 0;

    using member_info_modifier_type = std::function<void(registration_committee_member_object&)>;
    virtual void update_member_info(const registration_committee_member_object&,
                                    const member_info_modifier_type& modifier)
        = 0;
};

class dbs_registration_committee : public dbs_base, public registration_committee_service_i
{
    friend class dbservice_dbs_factory;

protected:
    explicit dbs_registration_committee(database& db);

public:
    committee_members_cref_type get_committee() const override;

    const registration_committee_member_object& get_member(const account_name_type&) const override;

    committee_members_cref_type create_committee(const std::vector<account_name_type>& accounts) override;

    using member_info_modifier_type = std::function<void(registration_committee_member_object&)>;
    void update_member_info(const registration_committee_member_object&,
                            const member_info_modifier_type& modifier) override;

    size_t get_members_count() const override;

    void add_member(const account_name_type&) override;
    void exclude_member(const account_name_type&) override;

    void change_add_member_quorum(const percent_type quorum) override;
    void change_exclude_member_quorum(const percent_type quorum) override;
    void change_base_quorum(const percent_type quorum) override;
    void change_transfer_quorum(const percent_type quorum) override;
    void change_top_budgets_quorum(const percent_type quorum) override;

    percent_type get_add_member_quorum() override;
    percent_type get_exclude_member_quorum() override;
    percent_type get_base_quorum() override;
    percent_type get_transfer_quorum() override;
    percent_type get_top_budgets_quorum() override;

    bool is_exists(const account_name_type&) const override;

private:
    const registration_committee_member_object& _add_member(const account_object&);

    void _exclude_member(const account_object&);
};

namespace utils {
bool is_quorum(size_t votes, size_t members_count, size_t quorum);
} // namespace utils

} // namespace chain
} // namespace scorum
