#include <scorum/chain/services/withdraw_scorumpower_route_statistic.hpp>

#include <scorum/chain/database/database.hpp>

#include <scorum/chain/schema/withdraw_scorumpower_objects.hpp>

namespace scorum {
namespace chain {

class dbs_withdraw_scorumpower_route_statistic_impl : public dbs_base
{
public:
    dbs_withdraw_scorumpower_route_statistic_impl(database& db)
        : dbs_base(db)
    {
    }

    template <typename IdType> bool is_exists(const IdType& from) const
    {
        return nullptr != db_impl().find<withdraw_scorumpower_route_statistic_object, by_destination>(from);
    }

    template <typename IdType> const withdraw_scorumpower_route_statistic_object& get(const IdType& from) const
    {
        try
        {
            return db_impl().get<withdraw_scorumpower_route_statistic_object, by_destination>(from);
        }
        FC_CAPTURE_AND_RETHROW((from))
    }
};

dbs_withdraw_scorumpower_route_statistic::dbs_withdraw_scorumpower_route_statistic(database& db)
    : dbs_base(db)
    , _impl(new dbs_withdraw_scorumpower_route_statistic_impl(db))
{
}

dbs_withdraw_scorumpower_route_statistic::~dbs_withdraw_scorumpower_route_statistic()
{
}

bool dbs_withdraw_scorumpower_route_statistic::is_exists(const account_id_type& from) const
{
    return _impl->is_exists(from);
}

bool dbs_withdraw_scorumpower_route_statistic::is_exists(const dev_committee_id_type& from) const
{
    return _impl->is_exists(from);
}

const withdraw_scorumpower_route_statistic_object&
dbs_withdraw_scorumpower_route_statistic::get(const account_id_type& from) const
{
    return _impl->get(from);
}

const withdraw_scorumpower_route_statistic_object&
dbs_withdraw_scorumpower_route_statistic::get(const dev_committee_id_type& from) const
{
    return _impl->get(from);
}

const withdraw_scorumpower_route_statistic_object&
dbs_withdraw_scorumpower_route_statistic::create(const modifier_type& modifier)
{
    return db_impl().create<withdraw_scorumpower_route_statistic_object>(
        [&](withdraw_scorumpower_route_statistic_object& o) { modifier(o); });
}

void dbs_withdraw_scorumpower_route_statistic::update(const withdraw_scorumpower_route_statistic_object& obj,
                                                  const modifier_type& modifier)
{
    db_impl().modify(obj, [&](withdraw_scorumpower_route_statistic_object& o) { modifier(o); });
}

void dbs_withdraw_scorumpower_route_statistic::remove(const withdraw_scorumpower_route_statistic_object& obj)
{
    db_impl().remove(obj);
}

} // namespace chain
} // namespace scorum
