#include <scorum/chain/services/withdraw_vesting.hpp>

#include <scorum/chain/database.hpp>

#include <scorum/chain/schema/withdraw_vesting_route_objects.hpp>

namespace scorum {
namespace chain {

class dbs_withdraw_vesting_impl : public dbs_base
{
public:
    dbs_withdraw_vesting_impl(database& db)
        : dbs_base(db)
    {
    }

    template <typename IdType> bool is_exists(const IdType& from) const
    {
        return nullptr != db_impl().find<withdraw_vesting_object, by_destination>(from);
    }

    template <typename IdType> const withdraw_vesting_object& get(const IdType& from) const
    {
        try
        {
            return db_impl().get<withdraw_vesting_object, by_destination>(from);
        }
        FC_CAPTURE_AND_RETHROW((from))
    }
};

dbs_withdraw_vesting::dbs_withdraw_vesting(database& db)
    : dbs_base(db)
    , _impl(new dbs_withdraw_vesting_impl(db))
{
}

dbs_withdraw_vesting::~dbs_withdraw_vesting()
{
}

bool dbs_withdraw_vesting::is_exists(account_id_type from) const
{
    return _impl->is_exists(from);
}

bool dbs_withdraw_vesting::is_exists(dev_committee_id_type from) const
{
    return _impl->is_exists(from);
}

const withdraw_vesting_object& dbs_withdraw_vesting::get(account_id_type from) const
{
    return _impl->get(from);
}

const withdraw_vesting_object& dbs_withdraw_vesting::get(dev_committee_id_type from) const
{
    return _impl->get(from);
}

const withdraw_vesting_object& dbs_withdraw_vesting::create(const modifier_type& modifier)
{
    return db_impl().create<withdraw_vesting_object>([&](withdraw_vesting_object& o) { modifier(o); });
}

void dbs_withdraw_vesting::update(const withdraw_vesting_object& obj, const modifier_type& modifier)
{
    db_impl().modify(obj, [&](withdraw_vesting_object& o) { modifier(o); });
}

void dbs_withdraw_vesting::remove(const withdraw_vesting_object& obj)
{
    db_impl().remove(obj);
}

} // namespace chain
} // namespace scorum
