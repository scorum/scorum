#include <chainbase/chainbase.hpp>

#define SHARED_MEMORY_FILE "shared_memory.bin"
#define SHARED_MEMORY_META_FILE "shared_memory.meta"

namespace chainbase {

database::~database()
{
}

void database::check_dir_existance(const boost::filesystem::path& dir, bool read_only)
{
    if (!boost::filesystem::exists(dir) && read_only)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("database file not found at " + dir.native()));
    }

    boost::filesystem::create_directories(dir);
}

void database::create_meta_file(const boost::filesystem::path& file, bool read_only)
{
    ilog("Try to open meta file");

    if (boost::filesystem::exists(file))
    {
        if (read_only)
        {
            _meta.reset(new boost::interprocess::managed_mapped_file(boost::interprocess::open_only,
                                                                     file.generic_string().c_str()));
        }
        else
        {
            _meta.reset(new boost::interprocess::managed_mapped_file(boost::interprocess::open_read_only,
                                                                     file.generic_string().c_str()));
        }

        set_read_write_mutex_manager(_meta->find<read_write_mutex_manager>("rw_manager").first);
    }
    else
    {
        _meta.reset(new boost::interprocess::managed_mapped_file(
            boost::interprocess::create_only, file.generic_string().c_str(), sizeof(read_write_mutex_manager) * 2));

        set_read_write_mutex_manager(_meta->find_or_construct<read_write_mutex_manager>("rw_manager")());
    }
}

void database::open(const boost::filesystem::path& dir, uint32_t flags, uint64_t shared_file_size)
{
    bool read_only = !(flags & database::read_write);

    check_dir_existance(dir, read_only);

    if (_data_dir != dir)
        close();

    _data_dir = dir;

    create_segment_file(boost::filesystem::absolute(dir / SHARED_MEMORY_FILE), read_only, shared_file_size);

    create_meta_file(boost::filesystem::absolute(dir / SHARED_MEMORY_META_FILE), read_only);

    // create lock on meta file
    if (!read_only)
    {
        _flock = boost::interprocess::file_lock((dir / SHARED_MEMORY_META_FILE).generic_string().c_str());
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
    _data_dir = boost::filesystem::path();
}

void database::wipe()
{
    boost::filesystem::path dir = _data_dir;
    close();
    boost::filesystem::remove_all(dir / SHARED_MEMORY_FILE);
    boost::filesystem::remove_all(dir / SHARED_MEMORY_META_FILE);
    _index_map.clear();
}

} // namespace chainbase
