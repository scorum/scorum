#include <scorum/chain/services/account_registration_bonus.hpp>

namespace scorum {
namespace chain {

dbs_account_registration_bonus::dbs_account_registration_bonus(database& db)
    : base_service_type(db)
{
}

void dbs_account_registration_bonus::remove_if_exist(const account_name_type& name)
{
    const account_registration_bonus_object* pobj = find_by<by_account>(name);
    if (pobj)
    {
        remove(*pobj);
    }
}

// TODO
}
}
