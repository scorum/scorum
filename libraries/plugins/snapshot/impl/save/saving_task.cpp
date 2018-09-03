#include <snapshot/save/saving_task.hpp>

#include <stack>

#include <fc/exception/exception.hpp>

#include <scorum/chain/database/database.hpp>
#include <chainbase/db_state.hpp>
#include <scorum/protocol/block.hpp>
#include <scorum/chain/services/dynamic_global_property.hpp>

#include <scorum/snapshot/snapshot_helper.hpp>
#include <snapshot/save/save_file.hpp>

namespace scorum {
namespace snapshot {

using chainbase::db_state;
using protocol::signed_block;

class saving_task_impl
{
public:
    saving_task_impl(chain::database& db)
        : _db(db)
        , _state(static_cast<db_state&>(db))
        , _dprops_service(db.dynamic_global_property_service())
    {
    }

    void set_snapshot_dir(const fc::path p)
    {
        _snapshot_dir = p;
    }

    void schedule_snapshot(uint32_t number)
    {
        if (number != get_snapshot_scheduled_number())
            _planned_snapshots.emplace(number);
    }

    void apply(const signed_block& block)
    {
        if (!is_snapshot_scheduled())
        {
#ifdef SNAPSHOT_SAVE_MULTIPLE_SNAPSHOTS
            if (!check_snapshot_task(block))
#endif // SNAPSHOT_SAVE_MULTIPLE_SNAPSHOTS
                return;
        }

        auto number = get_snapshot_scheduled_number();
        if (number && number != block.block_num())
            return;

        _planned_snapshots.pop();

        fc::path snapshot_path = create_snapshot_path(_dprops_service, _snapshot_dir);
        try
        {
            FC_ASSERT(_db.flags() & to_underlying(chain::database::open_flags::read_write),
                      "Loading supports only read/write database mode");

            ilog("Making snapshot for block ${n} to file ${f}.",
                 ("n", block.block_num())("f", snapshot_path.generic_string()));

            save_algorithm(_state, block, snapshot_path).save();
            return;
        }
        FC_CAPTURE_AND_LOG((number)(snapshot_path))
        fc::remove_all(snapshot_path);
    }

private:
    bool is_snapshot_scheduled() const
    {
        return !_planned_snapshots.empty();
    }

    uint32_t get_snapshot_scheduled_number() const
    {
        return _planned_snapshots.empty() ? uint32_t(-1) : _planned_snapshots.top();
    }

#ifdef SNAPSHOT_SAVE_MULTIPLE_SNAPSHOTS
    bool check_snapshot_task(const protocol::signed_block& block)
    {
        fc::path task_file_path = _snapshot_dir / fc::to_string(block.block_num());
        if (fc::exists(task_file_path))
        {
            schedule_snapshot(block.block_num());
        }

        return is_snapshot_scheduled();
    }
#endif // SNAPSHOT_SAVE_MULTIPLE_SNAPSHOTS

    chain::database& _db;
    chainbase::db_state& _state;
    chain::dynamic_global_property_service_i& _dprops_service;

    fc::path _snapshot_dir;

    std::stack<uint32_t> _planned_snapshots;
};

saving_task::saving_task(chain::database& db)
    : _impl(new saving_task_impl(db))
{
}
saving_task::~saving_task()
{
}

void saving_task::set_snapshot_dir(const fc::path p)
{
    FC_ASSERT(p != fc::path(), "Empty path is not allowed");
    _impl->set_snapshot_dir(p);
}

void saving_task::schedule_snapshot(uint32_t number)
{
    _impl->schedule_snapshot(number);
}

void saving_task::apply(const protocol::signed_block& block)
{
    _impl->apply(block);
}
}
}
