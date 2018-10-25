#include <scorum/chain/dba/db_accessor_factory.hpp>
#include <scorum/chain/dba/db_accessor.hpp>

#include <scorum/chain/schema/game_object.hpp>
#include <scorum/chain/schema/proposal_object.hpp>
#include <scorum/chain/schema/betting_property_object.hpp>
#include <scorum/chain/schema/bet_objects.hpp>
#include <scorum/chain/schema/registration_objects.hpp>
#include <scorum/chain/schema/account_objects.hpp>
#include <scorum/chain/schema/comment_objects.hpp>

// clang-format off
#define DB_TYPES                                                                                                       \
    (game_object)                                                                                                      \
    (proposal_object)                                                                                                  \
    (betting_property_object)                                                                                          \
    (pending_bet_object)                                                                                               \
    (matched_bet_object)                                                                                               \
    (registration_pool_object)                                                                                         \
    (registration_committee_member_object)                                                                             \
    (reg_pool_sp_delegation_object)                                                                                    \
    (comment_object)
// clang-format on

#define INSTANTIATE_DBA_FACTORY_METHODS(_1, _2, TYPE)                                                                  \
    template db_accessor<TYPE>& db_accessor_factory::get_dba<TYPE>() const;

namespace scorum {
namespace chain {
namespace dba {

db_accessor_factory::db_accessor_factory(db_index& db)
    : _db(db)
{
}

template <typename TObject> db_accessor<TObject>& db_accessor_factory::get_dba() const
{
    auto it = _db_accessors.emplace(boost::typeindex::type_id<TObject>(), db_accessor<TObject>{ _db }).first;

    return boost::any_cast<db_accessor<TObject>&>(it->second);
}

BOOST_PP_SEQ_FOR_EACH(INSTANTIATE_DBA_FACTORY_METHODS, , DB_TYPES)
}
}
}
