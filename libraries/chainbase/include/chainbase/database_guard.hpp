#pragma once

#include <atomic>
#include <array>
#include <typeinfo>

#include <boost/interprocess/sync/interprocess_sharable_mutex.hpp>
#include <boost/interprocess/sync/sharable_lock.hpp>
#include <boost/thread/locks.hpp>

#include <fc/exception/exception.hpp>
#include <fc/scoped_increment.hpp>

#ifndef CHAINBASE_NUM_RW_LOCKS
#define CHAINBASE_NUM_RW_LOCKS 10
#endif

#define CHAINBASE_REQUIRE_READ_LOCK(t) require_read_lock(__FUNCTION__, typeid(t).name())
#define CHAINBASE_REQUIRE_WRITE_LOCK(t) require_write_lock(__FUNCTION__, typeid(t).name())

namespace chainbase {

typedef boost::interprocess::interprocess_sharable_mutex read_write_mutex;
typedef boost::interprocess::sharable_lock<read_write_mutex> read_lock;
typedef boost::unique_lock<read_write_mutex> write_lock;

//////////////////////////////////////////////////////////////////////////
class read_write_mutex_manager
{
public:
    read_write_mutex_manager();
    ~read_write_mutex_manager();

    void next_lock();
    read_write_mutex& current_lock();
    uint32_t current_lock_num();

private:
    std::array<read_write_mutex, CHAINBASE_NUM_RW_LOCKS> _locks;
    std::atomic<uint32_t> _current_lock;
};

//////////////////////////////////////////////////////////////////////////
class database_guard
{
protected:
    read_write_mutex_manager* _rw_manager = nullptr;

    int32_t _read_lock_count = 0;
    int32_t _write_lock_count = 0;
    bool _enable_require_locking = false;

    bool _read_only = false;

public:
    virtual ~database_guard();

    void set_require_locking(bool enable_require_locking);

    void require_lock_fail(const char* method, const char* lock_type, const char* tname) const;

    void require_read_lock(const char* method, const char* tname) const;

    void require_write_lock(const char* method, const char* tname);

    void set_read_write_mutex_manager(read_write_mutex_manager* manager);

    template <typename Lambda>
    auto with_read_lock(Lambda&& callback, uint64_t wait_micro = 1000000) -> decltype((*(Lambda*)nullptr)())
    {
        FC_ASSERT(_rw_manager);

        read_lock lock(_rw_manager->current_lock(), boost::interprocess::defer_lock_type());
        SCOPED_INCREMENT(_read_lock_count);

        if (!wait_micro)
        {
            lock.lock();
        }
        else
        {
            if (!lock.timed_lock(boost::posix_time::microsec_clock::universal_time()
                                 + boost::posix_time::microseconds(wait_micro)))
                BOOST_THROW_EXCEPTION(std::runtime_error("unable to acquire lock"));
        }

        return callback();
    }

    template <typename Lambda>
    auto with_write_lock(Lambda&& callback, uint64_t wait_micro = 1000000) -> decltype((*(Lambda*)nullptr)())
    {
        if (_read_only)
            BOOST_THROW_EXCEPTION(std::logic_error("cannot acquire write lock on read-only process"));

        FC_ASSERT(_rw_manager);

        write_lock lock(_rw_manager->current_lock(), boost::defer_lock_t());
        SCOPED_INCREMENT(_write_lock_count);

        if (!wait_micro)
        {
            lock.lock();
        }
        else
        {
            while (!lock.timed_lock(boost::posix_time::microsec_clock::universal_time()
                                    + boost::posix_time::microseconds(wait_micro)))
            {
                _rw_manager->next_lock();
                std::cerr << "Lock timeout, moving to lock " << _rw_manager->current_lock_num() << std::endl;
                lock = write_lock(_rw_manager->current_lock(), boost::defer_lock_t());
            }
        }

        return callback();
    }
};
}
