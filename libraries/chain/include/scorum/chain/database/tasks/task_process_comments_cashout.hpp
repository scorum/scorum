#pragma once

#include <scorum/chain/database/tasks/tasks.hpp>

#include <scorum/chain/services/comment.hpp>

namespace scorum {
namespace chain {
namespace database_ns {

struct task_process_comments_cashout_impl : public task
{
    virtual tasks get_type() const
    {
        return task_process_comments_cashout;
    }

    virtual void apply(task_context&);

private:
    uint128_t get_recent_claims(task_context& ctx, const comment_service_i::comment_refs_type&);

    share_type pay_for_comment(task_context& ctx, const comment_object& comment, const share_type& reward_tokens);

    share_type pay_curators(task_context& ctx, const comment_object& comment, share_type& max_rewards);
};
}
}
}
