#include <scorum/chain/dbs_escrow.hpp>
#include <scorum/chain/database.hpp>

#include <scorum/chain/scorum_objects.hpp>

#include <tuple>

using namespace scorum::protocol;

namespace scorum {
namespace chain {

dbs_escrow::dbs_escrow(database& db)
    : _base_type(db)
{
}

const escrow_object& dbs_escrow::get(const account_name_type& name, uint32_t escrow_id) const
{
    try
    {
        return db_impl().get<escrow_object, by_from_id>(boost::make_tuple(name, escrow_id));
    }
    FC_CAPTURE_AND_RETHROW((name)(escrow_id))
}

const escrow_object& dbs_escrow::create(uint32_t escrow_id,
                                        const account_name_type& from,
                                        const account_name_type& to,
                                        const account_name_type& agent,
                                        const time_point_sec& ratification_deadline,
                                        const time_point_sec& escrow_expiration,
                                        const asset& scorum_amount,
                                        const asset& pending_fee)
{
    const auto& new_escrow = db_impl().create<escrow_object>([&](escrow_object& esc) {
        esc.escrow_id = escrow_id;
        esc.from = from;
        esc.to = to;
        esc.agent = agent;
        esc.ratification_deadline = ratification_deadline;
        esc.escrow_expiration = escrow_expiration;
        esc.scorum_balance = scorum_amount;
        esc.pending_fee = pending_fee;
    });

    return new_escrow;
}

void dbs_escrow::update(const escrow_object& escrow, modifier_type modifier)
{
    db_impl().modify(escrow, [&](escrow_object& e) { modifier(e); });
}

void dbs_escrow::remove(const escrow_object& escrow)
{
    db_impl().remove(escrow);
}

} // namespace chain
} // namespace scorum
