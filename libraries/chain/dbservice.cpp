#include <scorum/chain/dbservice.hpp>

namespace scorum {
namespace chain {

    const account_object&  i_dbservice::get_account(  const account_name_type& name )const
    {
        return _db.get_account(name);
    }
}
}
