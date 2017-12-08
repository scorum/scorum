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

/** DB service for operations with registration_pool object
 *  --------------------------------------------
*/
class dbs_registration_pool : public dbs_base
{
    friend class dbservice_dbs_factory;

protected:
    explicit dbs_registration_pool(database& db);

public:
    bool is_pool_exists() const;

    const registration_pool_object& get_pool() const;

    const registration_pool_object& create_pool(const genesis_state_type& genesis_state);

    asset allocate_cash(const account_name_type& committee_member);

private:
    share_type _calculate_per_reg(const registration_pool_object&);

    share_type _decrease_balance(const registration_pool_object&, const share_type&);

    bool _check_autoclose(const registration_pool_object&);

    void _close(const registration_pool_object&);
};
}
}
