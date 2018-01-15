#pragma once

#include <boost/filesystem.hpp>
#include <boost/interprocess/sync/file_lock.hpp>

#include <chainbase/database_index.hpp>

namespace chainbase {

namespace bfs = boost::filesystem;

class database : public database_index
{
    bip::file_lock _flock;

    bfs::path _data_dir;

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
    void wipe(const bfs::path& dir);
};

} // namespace chainbase
