#include <boost/config.hpp>
#include <boost/throw_exception.hpp>

#include <chainbase/database_guard.hpp>

namespace chainbase {

read_write_mutex_manager::read_write_mutex_manager()
{
    _current_lock = 0;
}

read_write_mutex_manager::~read_write_mutex_manager()
{
}

void read_write_mutex_manager::next_lock()
{
    ++_current_lock;
    new (&_locks[_current_lock % CHAINBASE_NUM_RW_LOCKS]) read_write_mutex();
}

read_write_mutex& read_write_mutex_manager::current_lock()
{
    return _locks[_current_lock % CHAINBASE_NUM_RW_LOCKS];
}

uint32_t read_write_mutex_manager::current_lock_num()
{
    return _current_lock;
}

//////////////////////////////////////////////////////////////////////////
database_guard::~database_guard()
{
}

void database_guard::set_require_locking(bool enable_require_locking)
{
    _enable_require_locking = enable_require_locking;
}

void database_guard::require_lock_fail(const char* method, const char* lock_type, const char* tname) const
{
    std::string err_msg = "database_guard::" + std::string(method) + " require_" + std::string(lock_type)
        + "_lock() failed on type " + std::string(tname);
    std::cerr << err_msg << std::endl;
    BOOST_THROW_EXCEPTION(std::runtime_error(err_msg));
}

void database_guard::require_read_lock(const char* method, const char* tname) const
{
    if (BOOST_UNLIKELY(_enable_require_locking & /*_read_only & */ (_read_lock_count <= 0)))
        require_lock_fail(method, "read", tname);
}

void database_guard::require_write_lock(const char* method, const char* tname)
{
    if (BOOST_UNLIKELY(_enable_require_locking & (_write_lock_count <= 0)))
        require_lock_fail(method, "write", tname);
}

void database_guard::set_read_write_mutex_manager(read_write_mutex_manager* manager)
{
    FC_ASSERT(manager, "could not set read_write_lock_manager, it must not be NULL");

    _rw_manager = manager;
}
}
