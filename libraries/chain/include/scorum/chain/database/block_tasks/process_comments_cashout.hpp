#pragma once

#include <scorum/chain/database/block_tasks/block_tasks.hpp>

#include <scorum/chain/services/comment.hpp>
#include <scorum/chain/schema/scorum_objects.hpp>
#include <scorum/rewards_math/formulas.hpp>

namespace scorum {
namespace chain {
namespace database_ns {

using scorum::rewards_math::shares_vector_type;

struct process_comments_cashout : public block_task
{
    virtual void on_apply(block_task_context&);

private:
    shares_vector_type get_total_rshares(block_task_context& ctx, const comment_service_i::comment_refs_type&);

    asset pay_for_comment(block_task_context& ctx,
                          const comment_object& comment,
                          const asset& reward,
                          const asset& commenting_reward);

    asset pay_curators(block_task_context& ctx, const comment_object& comment, asset& max_rewards);
};
}
}
}
