#pragma once

#include <scorum/protocol/proposal_operations.hpp>

#include <scorum/chain/services/dbs_base.hpp>
#include <vector>
#include <set>
#include <functional>

#include <scorum/chain/schema/registration_objects.hpp>

namespace scorum {
namespace chain {

struct registration_committee_service_i : public scorum::protocol::registration_committee
{
    using registration_committee_member_refs_type = std::vector<registration_committee_member_object::cref_type>;

    virtual registration_committee_member_refs_type get_committee() const = 0;

    virtual const registration_committee_member_object& get_member(const account_name_type&) const = 0;

    virtual registration_committee_member_refs_type create_committee(const std::vector<account_name_type>& accounts)
        = 0;

    using member_info_modifier_type = std::function<void(registration_committee_member_object&)>;
    virtual void update_member_info(const registration_committee_member_object&,
                                    const member_info_modifier_type& modifier)
        = 0;

    virtual size_t get_members_count() const = 0;
};

/** DB service for operations with registration_committee_* objects
 *  --------------------------------------------
 */
class dbs_registration_committee : public dbs_base, public registration_committee_service_i
{
    friend class dbservice_dbs_factory;

protected:
    explicit dbs_registration_committee(database& db);

public:
    using registration_committee_member_refs_type = std::vector<registration_committee_member_object::cref_type>;

    registration_committee_member_refs_type get_committee() const override;

    const registration_committee_member_object& get_member(const account_name_type&) const override;

    registration_committee_member_refs_type create_committee(const std::vector<account_name_type>& accounts) override;

    void add_member(const account_name_type&) override;

    void exclude_member(const account_name_type&) override;

    using member_info_modifier_type = std::function<void(registration_committee_member_object&)>;
    void update_member_info(const registration_committee_member_object&,
                            const member_info_modifier_type& modifier) override;

    bool is_exists(const account_name_type&) const override;

    size_t get_members_count() const override;

    void change_add_member_quorum(const uint32_t quorum) override;
    void change_exclude_member_quorum(const uint32_t quorum) override;
    void change_quorum(const uint32_t quorum) override;

    int get_add_member_quorum() override;
    int get_exclude_member_quorum() override;
    int get_base_quorum() override;

private:
    const registration_committee_member_object& _add_member(const account_object&);

    void _exclude_member(const account_object&);
};

struct development_committee_service_i : public development_committee
{
};

struct dbs_development_committee : public dbs_base, public development_committee_service_i
{
    void add_member(const account_name_type&) override;
    void exclude_member(const account_name_type&) override;

    void change_add_member_quorum(const uint32_t quorum) override;
    void change_exclude_member_quorum(const uint32_t quorum) override;
    void change_quorum(const uint32_t quorum) override;

    int get_add_member_quorum() override;
    int get_exclude_member_quorum() override;
    int get_base_quorum() override;

    bool is_exists(const account_name_type&) const override;

private:
    friend class dbservice_dbs_factory;

protected:
    explicit dbs_development_committee(database& db);
};

namespace utils {
bool is_quorum(size_t votes, size_t members_count, size_t quorum);
} // namespace utils

} // namespace chain
} // namespace scorum
