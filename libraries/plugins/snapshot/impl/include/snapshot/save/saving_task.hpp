#pragma once

#include <fc/filesystem.hpp>

namespace scorum {
namespace chain {
class database;
}
namespace protocol {
class signed_block;
}
namespace snapshot {

class saving_task_impl;

class saving_task
{
public:
    explicit saving_task(chain::database& db);
    ~saving_task();

    void set_snapshot_dir(const fc::path);
    void schedule_snapshot(uint32_t number = 0);
    void apply(const protocol::signed_block&);

private:
    std::unique_ptr<saving_task_impl> _impl;
};
}
}
