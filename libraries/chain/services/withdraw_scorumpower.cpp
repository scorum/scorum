#include <scorum/chain/services/withdraw_scorumpower.hpp>

#include <scorum/chain/database/database.hpp>

#include <scorum/chain/schema/withdraw_scorumpower_objects.hpp>

namespace scorum {
namespace chain {

class dbs_withdraw_scorumpower_impl : public dbs_base
{
public:
    dbs_withdraw_scorumpower_impl(database& db)
        : dbs_base(db)
    {
    }

    template <typename IdType> bool is_exists(const IdType& from) const
    {
        return nullptr != db_impl().find<withdraw_scorumpower_object, by_destination>(from);
    }

    template <typename IdType> const withdraw_scorumpower_object& get(const IdType& from) const
    {
        try
        {
            return db_impl().get<withdraw_scorumpower_object, by_destination>(from);
        }
        FC_CAPTURE_AND_RETHROW((from))
    }
};

dbs_withdraw_scorumpower::dbs_withdraw_scorumpower(database& db)
    : dbs_base(db)
    , _impl(new dbs_withdraw_scorumpower_impl(db))
{
}

dbs_withdraw_scorumpower::~dbs_withdraw_scorumpower()
{
}

bool dbs_withdraw_scorumpower::is_exists(const account_id_type& from) const
{
    return _impl->is_exists(from);
}

bool dbs_withdraw_scorumpower::is_exists(const dev_committee_id_type& from) const
{
    return _impl->is_exists(from);
}

const withdraw_scorumpower_object& dbs_withdraw_scorumpower::get(const account_id_type& from) const
{
    return _impl->get(from);
}

const withdraw_scorumpower_object& dbs_withdraw_scorumpower::get(const dev_committee_id_type& from) const
{
    return _impl->get(from);
}

dbs_withdraw_scorumpower::withdraw_scorumpower_refs_type
dbs_withdraw_scorumpower::get_until(const time_point_sec& until) const
{
    withdraw_scorumpower_refs_type ret;

    const auto& idx = db_impl().get_index<withdraw_scorumpower_index>().indices().get<by_next_vesting_withdrawal>();
    auto it = idx.cbegin();
    const auto it_end = idx.cend();
    while (it != it_end && it->next_vesting_withdrawal <= until)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

asset dbs_withdraw_scorumpower::get_withdraw_rest(const account_id_type& from) const
{
    if (!is_exists(from))
        return asset(0, SP_SYMBOL);
    const withdraw_scorumpower_object& wvo = get(from);
    return wvo.to_withdraw - wvo.withdrawn;
}

const withdraw_scorumpower_object& dbs_withdraw_scorumpower::create(const modifier_type& modifier)
{
    return db_impl().create<withdraw_scorumpower_object>([&](withdraw_scorumpower_object& o) { modifier(o); });
}

void dbs_withdraw_scorumpower::update(const withdraw_scorumpower_object& obj, const modifier_type& modifier)
{
    db_impl().modify(obj, [&](withdraw_scorumpower_object& o) { modifier(o); });
}

void dbs_withdraw_scorumpower::remove(const withdraw_scorumpower_object& obj)
{
    db_impl().remove(obj);
}

} // namespace chain
} // namespace scorum
