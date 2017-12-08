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
    bool is_committee_exists() const;

    const registration_committee_object& get_committee() const;

    const registration_committee_member_object& get_member(const account_name_type&) const;

    uint64_t get_member_count() const;

    const registration_committee_object& create_committee(const genesis_state_type& genesis_state);

    const registration_committee_member_object& add_member(const account_name_type&);

    void exclude_member(const account_name_type&);

    void update_cash_info(const registration_committee_member_object&, share_type per_reg, bool reset);

private:
    bool _check_member_exist(const account_name_type&) const;

    const registration_committee_member_object& _add_member(const account_object&);

    void _exclude_member(const account_object&);
};
}
}
