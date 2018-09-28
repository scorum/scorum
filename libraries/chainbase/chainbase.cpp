#include <chainbase/chainbase.hpp>

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

void database::create_meta_file(const boost::filesystem::path& file)
{
    ilog("Try to open meta file in read/write mode");

    if (boost::filesystem::exists(file))
    {
        _meta.reset(new boost::interprocess::managed_mapped_file(boost::interprocess::open_only,
                                                                 file.generic_string().c_str()));

        set_read_write_mutex_manager(_meta->find<read_write_mutex_manager>("rw_manager").first);
    }
    else
    {
        _meta.reset(new boost::interprocess::managed_mapped_file(
            boost::interprocess::create_only, file.generic_string().c_str(), sizeof(read_write_mutex_manager) * 2));

        set_read_write_mutex_manager(_meta->find_or_construct<read_write_mutex_manager>("rw_manager")());
    }
}

boost::filesystem::path database::shared_memory_path(const boost::filesystem::path& data_dir)
{
    return data_dir / "shared_memory.bin";
}

boost::filesystem::path database::shared_memory_meta_path(const boost::filesystem::path& data_dir)
{
    return data_dir / "shared_memory.meta";
}

void database::open(const boost::filesystem::path& dir, uint32_t flags, uint64_t shared_file_size)
{
    _dir = dir;
    _flags = flags;

    bool read_only = !(flags & scorum::to_underlying(open_flags::read_write));

    check_dir_existance(dir, read_only);

    close();

    create_segment_file(shared_memory_path(dir), read_only, shared_file_size);

    create_meta_file(shared_memory_meta_path(dir));

    // create lock on meta file
    if (!read_only)
    {
        _flock = boost::interprocess::file_lock(shared_memory_meta_path(dir).generic_string().c_str());
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
}

void database::wipe(const boost::filesystem::path& dir)
{
    close();

    boost::filesystem::remove_all(shared_memory_path(dir));
    boost::filesystem::remove_all(shared_memory_meta_path(dir));
    _index_map.clear();
}

} // namespace chainbase
