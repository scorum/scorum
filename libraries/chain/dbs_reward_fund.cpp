#include <scorum/chain/dbs_reward_fund.hpp>
#include <scorum/chain/database.hpp>
#include <scorum/chain/scorum_objects.hpp>

namespace scorum {
namespace chain {

dbs_reward_fund::dbs_reward_fund(database& db)
    : dbs_base(db)
{
}

const reward_fund_object& dbs_reward_fund::get_reward_fund() const
{
    return db_impl().get<reward_fund_object>();
}

} // namespace scorum
} // namespace chain
