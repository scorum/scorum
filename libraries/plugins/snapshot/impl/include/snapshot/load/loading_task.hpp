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

class loading_task_impl;

class loading_task
{
public:
    explicit loading_task(chain::database& db);
    ~loading_task();

    void set_snapshot_dir(const fc::path);
    void set_snapshot_file(const fc::path);
    uint32_t get_snapshot_number() const;
    void apply(const protocol::signed_block&);

private:
    std::unique_ptr<loading_task_impl> _impl;
};
}
}
