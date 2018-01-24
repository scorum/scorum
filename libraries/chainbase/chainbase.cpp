#include <chainbase/chainbase.hpp>

#define SHARED_MEMORY_FILE "shared_memory.bin"
#define SHARED_MEMORY_META_FILE "shared_memory.meta"

namespace chainbase {

database::~database()
{
}

void database::check_dir_existance(const bfs::path& dir, bool read_only)
{
    if (!bfs::exists(dir) && read_only)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("database file not found at " + dir.native()));
    }

    bfs::create_directories(dir);
}

void database::create_meta_file(const bfs::path& file)
{
    if (bfs::exists(file))
    {
        _meta.reset(new bip::managed_mapped_file(bip::open_only, file.generic_string().c_str()));

        set_read_write_mutex_manager(_meta->find<read_write_mutex_manager>("rw_manager").first);
    }
    else
    {
        _meta.reset(new bip::managed_mapped_file(bip::create_only, file.generic_string().c_str(),
                                                 sizeof(read_write_mutex_manager) * 2));

        set_read_write_mutex_manager(_meta->find_or_construct<read_write_mutex_manager>("rw_manager")());
    }
}

void database::open(const bfs::path& dir, uint32_t flags, uint64_t shared_file_size)
{
    bool read_only = !(flags & database::read_write);

    check_dir_existance(dir, read_only);

    if (_data_dir != dir)
        close();

    _data_dir = dir;

    create_segment_file(bfs::absolute(dir / SHARED_MEMORY_FILE), read_only, shared_file_size);

    create_meta_file(bfs::absolute(dir / SHARED_MEMORY_META_FILE));

    // create lock on meta file
    if (!read_only)
    {
        _flock = bip::file_lock((dir / SHARED_MEMORY_META_FILE).generic_string().c_str());
        if (!_flock.try_lock())
            BOOST_THROW_EXCEPTION(std::runtime_error("could not gain write access to the shared memory file"));
    }
}

void database::flush()
{
    flush_segment_file();

    if (_meta)
        _meta->flush();
}

void database::close()
{
    close_segment_file();

    _meta.reset();
    _data_dir = bfs::path();
}

void database::wipe()
{
    bfs::path dir = _data_dir;
    close();
    bfs::remove_all(dir / SHARED_MEMORY_FILE);
    bfs::remove_all(dir / SHARED_MEMORY_META_FILE);
    _index_map.clear();
}

} // namespace chainbase
