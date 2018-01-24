#include <scorum/witness/witness_operations.hpp>
#include <scorum/witness/witness_objects.hpp>

#include <scorum/chain/schema/comment_objects.hpp>

namespace scorum {
namespace witness {

void enable_content_editing_evaluator::do_apply(const enable_content_editing_operation& o)
{
    try
    {
        auto edit_lock = db().find<content_edit_lock_object, by_account>(o.account);

        if (edit_lock == nullptr)
        {
            db().create<content_edit_lock_object>([&](content_edit_lock_object& lock) {
                lock.account = o.account;
                lock.lock_time = o.relock_time;
            });
        }
        else
        {
            db().modify(*edit_lock, [&](content_edit_lock_object& lock) { lock.lock_time = o.relock_time; });
        }
    }
    FC_CAPTURE_AND_RETHROW((o))
}
}
} // scorum::witness
