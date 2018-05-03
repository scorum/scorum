#pragma once

#include <scorum/chain/database/block_tasks/block_tasks.hpp>

#include <scorum/chain/services/comment.hpp>
#include <scorum/chain/schema/scorum_objects.hpp>
#include <scorum/rewards_math/formulas.hpp>

namespace scorum {
namespace chain {
namespace database_ns {

using scorum::rewards_math::shares_vector_type;

struct comment_payout_result
{
    asset total_claimed_reward;
    asset parent_author_reward;
};

struct process_comments_cashout : public block_task
{
    virtual void on_apply(block_task_context&);

private:
    shares_vector_type get_total_rshares(block_task_context& ctx, const comment_service_i::comment_refs_type&);

    comment_payout_result pay_for_comment(block_task_context& ctx,
                                          const comment_object& comment,
                                          const asset& from_voting_payout,
                                          const asset& from_commenting_payout);

    asset pay_curators(block_task_context& ctx, const comment_object& comment, asset& max_rewards);

    struct by_depth_less
    {
        bool operator()(const comment_object& lhs, const comment_object& rhs) const;
    };

    using comment_refs_type = comment_service_i::comment_refs_type;
    using comment_refs_set = std::set<comment_refs_type::value_type, by_depth_less>;

    comment_refs_type collect_parents(block_task_context& ctx, const comment_refs_type& comments);
};
}
}
}
