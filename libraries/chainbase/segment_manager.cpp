#include <boost/array.hpp>
#include <boost/filesystem.hpp>
#include <fc/exception/exception.hpp>
#include <chainbase/segment_manager.hpp>

namespace chainbase {

struct environment_check
{
    environment_check()
    {
        memset(&compiler_version, 0, sizeof(compiler_version));
#ifdef WIN32
        snprintf(compiler_version.data(), compiler_version.size(), "%d", _MSC_FULL_VER);
#else
        memcpy(&compiler_version, __VERSION__, std::min<size_t>(strlen(__VERSION__), 256));
#endif
#ifndef NDEBUG
        debug = true;
#endif
#ifdef __APPLE__
        apple = true;
#endif
#ifdef WIN32
        windows = true;
#endif
    }
    friend bool operator==(const environment_check& a, const environment_check& b)
    {
        return std::make_tuple(a.compiler_version, a.debug, a.apple, a.windows)
            == std::make_tuple(b.compiler_version, b.debug, b.apple, b.windows);
    }

    boost::array<char, 256> compiler_version;
    bool debug = false;
    bool apple = false;
    bool windows = false;
};

//////////////////////////////////////////////////////////////////////////

void segment_manager::create_segment_file(const boost::filesystem::path& file,
                                          bool read_only,
                                          uint64_t shared_file_size)
{
    ilog("Try to open segment file");

    if (boost::filesystem::exists(file))
    {
        if (read_only)
        {
            _segment.reset(new boost::interprocess::managed_mapped_file(boost::interprocess::open_read_only,
                                                                        file.generic_string().c_str()));
        }
        else
        {
            auto existing_file_size = boost::filesystem::file_size(file);
            if (shared_file_size > existing_file_size)
            {
                if (!boost::interprocess::managed_mapped_file::grow(file.generic_string().c_str(),
                                                                    shared_file_size - existing_file_size))
                    BOOST_THROW_EXCEPTION(std::runtime_error("could not grow database file to requested size."));
            }

            _segment.reset(new boost::interprocess::managed_mapped_file(boost::interprocess::open_only,
                                                                        file.generic_string().c_str()));
        }

        _read_only = read_only;

        auto env = _segment->find<environment_check>("environment");
        if (!env.first || !(*env.first == environment_check()))
        {
            BOOST_THROW_EXCEPTION(
                std::runtime_error("database created by a different compiler, build, or operating system"));
        }
    }
    else
    {
        _segment.reset(new boost::interprocess::managed_mapped_file(boost::interprocess::create_only,
                                                                    file.generic_string().c_str(), shared_file_size));
        _segment->construct<environment_check>("environment")();
    }
}

void segment_manager::flush_segment_file()
{
    FC_ASSERT(_segment);
    _segment->flush();
}

void segment_manager::close_segment_file()
{
    _segment.reset();
}

size_t segment_manager::get_free_memory() const
{
    FC_ASSERT(_segment);
    return _segment->get_segment_manager()->get_free_memory();
}
}
