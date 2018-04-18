#pragma once

#include <scorum/chain/database/block_tasks/block_tasks.hpp>

#include <scorum/chain/services/comment.hpp>
#include <scorum/chain/schema/scorum_objects.hpp>
#include <scorum/rewards_math/formulas.hpp>

namespace scorum {
namespace chain {
namespace database_ns {

using scorum::rewards::share_types;

class process_comments_cashout_impl;

struct process_comments_cashout : public block_task
{
    virtual void on_apply(block_task_context&);

private:
    void apply_for_scr_fund(block_task_context&, process_comments_cashout_impl&);
    void apply_for_sp_fund(block_task_context&, process_comments_cashout_impl&);
};
}
}
}
