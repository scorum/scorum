#pragma once

#include <scorum/chain/dbs_base_impl.hpp>
#include <vector>
#include <set>
#include <functional>

#include <scorum/chain/registration_objects.hpp>

namespace scorum {
namespace chain {

/** DB service for operations with registration objects
 *  --------------------------------------------
*/
class dbs_registration : public dbs_base
{
    friend class dbservice_dbs_factory;

protected:
    explicit dbs_registration(database& db);

public:
    // TODO
};
}
}
