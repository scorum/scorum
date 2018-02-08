#pragma once

#include <scorum/chain/services/dbs_base.hpp>
#include <vector>
#include <set>
#include <functional>

#include <scorum/chain/schema/registration_objects.hpp>

namespace scorum {
namespace chain {

struct registration_committee_service_i
{
    using registration_committee_member_refs_type = std::vector<registration_committee_member_object::cref_type>;

    virtual registration_committee_member_refs_type get_committee() const = 0;

    virtual const registration_committee_member_object& get_member(const account_name_type&) const = 0;

    virtual registration_committee_member_refs_type create_committee(const std::vector<account_name_type>& accounts)
        = 0;

    virtual const registration_committee_member_object& add_member(const account_name_type&) = 0;

    virtual void exclude_member(const account_name_type&) = 0;

    using member_info_modifier_type = std::function<void(registration_committee_member_object&)>;
    virtual void update_member_info(const registration_committee_member_object&,
                                    const member_info_modifier_type& modifier)
        = 0;

    virtual bool is_exists(const account_name_type&) const = 0;

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

    const registration_committee_member_object& add_member(const account_name_type&) override;

    void exclude_member(const account_name_type&) override;

    using member_info_modifier_type = std::function<void(registration_committee_member_object&)>;
    void update_member_info(const registration_committee_member_object&,
                            const member_info_modifier_type& modifier) override;

    bool is_exists(const account_name_type&) const override;

    size_t get_members_count() const override;

private:
    const registration_committee_member_object& _add_member(const account_object&);

    void _exclude_member(const account_object&);
};

namespace utils {
bool is_quorum(size_t votes, size_t members_count, size_t quorum);
} // namespace utils

} // namespace chain
} // namespace scorum
