#include <snapshot/load/loading_task.hpp>

#include <fc/exception/exception.hpp>

#include <scorum/chain/database/database.hpp>
#include <chainbase/db_state.hpp>
#include <scorum/protocol/block.hpp>
#include <scorum/chain/services/dynamic_global_property.hpp>

#include <snapshot/load/load_file.hpp>

namespace scorum {
namespace snapshot {

class loading_task_impl
{
public:
    loading_task_impl(chain::database& db)
        : _db(db)
        , _state(static_cast<db_state&>(db))
        , _dprops_service(db.dynamic_global_property_service())
    {
    }

    void set_snapshot_dir(const fc::path p)
    {
        _snapshot_dir = p;
    }

    void set_snapshot_file(const fc::path p)
    {
        _snapshot_file = p;

        auto header = load_algorithm(_state, _snapshot_file).load_header();
        _start_apply_block_number = header.head_block_number;
        _start_apply_block_digest = header.head_block_digest;
    }

    uint32_t get_snapshot_number() const
    {
        return _start_apply_block_number;
    }

    void apply(const protocol::signed_block& block)
    {
        try
        {
            if (_start_apply_block_digest != digest_type())
            {
                if (block.digest() == _start_apply_block_digest)
                {
                    FC_ASSERT(_db.flags() & to_underlying(chain::database::open_flags::read_write),
                              "Saving supports only read/write database mode");

                    load_algorithm(_state, _snapshot_file).load();
                    _db.validate_invariants();
                    reset_snapshot_data();
                }
                else if (block.block_num() == _start_apply_block_number)
                {
                    elog("Invalid previous state. Can't appy snapshot for block ${n}:${h}",
                         ("n", _start_apply_block_number)("h", _start_apply_block_digest));
                    reset_snapshot_data();
                }
            }
        }
        FC_CAPTURE_AND_LOG((block))
    }

private:
    void reset_snapshot_data()
    {
        _start_apply_block_digest = digest_type();
        _start_apply_block_number = 0u;
    }

    chain::database& _db;
    chainbase::db_state& _state;
    chain::dynamic_global_property_service_i& _dprops_service;

    fc::path _snapshot_dir;
    fc::path _snapshot_file;

    uint32_t _start_apply_block_number = 0u;
    protocol::digest_type _start_apply_block_digest;
};

loading_task::loading_task(chain::database& db)
    : _impl(new loading_task_impl(db))
{
}
loading_task::~loading_task()
{
}

void loading_task::set_snapshot_dir(const fc::path p)
{
    FC_ASSERT(p != fc::path(), "Empty path is not allowed");
    _impl->set_snapshot_dir(p);
}

void loading_task::set_snapshot_file(const fc::path p)
{
    FC_ASSERT(fc::exists(p), "File does not exist");
    _impl->set_snapshot_file(p);
}

uint32_t loading_task::get_snapshot_number() const
{
    return _impl->get_snapshot_number();
}

void loading_task::apply(const protocol::signed_block& block)
{
    _impl->apply(block);
}
}
}
