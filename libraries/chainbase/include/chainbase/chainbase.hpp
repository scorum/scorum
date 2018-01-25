#pragma once

#include <boost/filesystem.hpp>
#include <boost/interprocess/sync/file_lock.hpp>

#include <chainbase/undo_db_state.hpp>

namespace chainbase {

namespace bfs = boost::filesystem;

class database : public undo_db_state
{
    bip::file_lock _flock;

    bfs::path _data_dir;

    std::unique_ptr<bip::managed_mapped_file> _meta;

private:
    void check_dir_existance(const bfs::path& dir, bool read_only);
    void create_meta_file(const bfs::path& file);

public:
    virtual ~database();

    enum open_flags
    {
        read_only = 0,
        read_write = 1
    };

    void open(const bfs::path& dir, uint32_t write = read_only, uint64_t shared_file_size = 0);
    void close();
    void flush();
    void wipe();
};

} // namespace chainbase
