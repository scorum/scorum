#pragma once

#include <scorum/chain/dbs_base_impl.hpp>
#include <vector>
#include <set>
#include <functional>

#include <scorum/chain/registration_objects.hpp>
#include <scorum/chain/account_object.hpp>
#include <scorum/chain/genesis_state.hpp>

namespace scorum {
namespace chain {

/** DB service for operations with registration_committee_* objects
 *  --------------------------------------------
*/
class dbs_registration_committee : public dbs_base
{
    friend class dbservice_dbs_factory;

protected:
    explicit dbs_registration_committee(database& db);

public:
    using registration_committee_member_refs_type = std::vector<std::reference_wrapper<const registration_committee_member_object>>;

    bool is_committee_exists() const;

    registration_committee_member_refs_type get_committee() const;

    const registration_committee_member_object& get_member(const account_name_type&) const;

    registration_committee_member_refs_type create_committee(const genesis_state_type& genesis_state);

    const registration_committee_member_object& add_member(const account_name_type&);

    void exclude_member(const account_name_type&);

    void update_cash_info(const registration_committee_member_object&, share_type per_reg, bool reset);

private:
    uint64_t _get_member_count() const;

    bool _check_member_exist(const account_name_type&) const;

    const registration_committee_member_object& _add_member(const account_object&);

    void _exclude_member(const account_object&);
};
}
}
