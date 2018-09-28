#include "genesis_tester.hpp"

#include <fc/log/logger.hpp>
#include <scorum/chain/database/database.hpp>
#include <scorum/chain/genesis/genesis_state.hpp>
#include <graphene/utilities/tempdir.hpp>

namespace scorum {

namespace util {

using namespace scorum::chain;

void test_database(const genesis_state_type& genesis, unsigned int shared_mem_mb_size)
{
    FC_ASSERT(shared_mem_mb_size < 1024);

    uint64_t test_shared_mem_size = (uint64_t)shared_mem_mb_size * 1024 * 1024;

    chain::database db(chain::database::opt_notify_virtual_op_applying);

    ilog("Test starting.");

    try
    {
        fc::temp_directory data_dir = fc::temp_directory(graphene::utilities::temp_directory_path());
        db.open(data_dir.path(), data_dir.path(), test_shared_mem_size, to_underlying(database::open_flags::read_write),
                genesis);

        ilog("Test completed. Database is opened successfully.");
    }
    catch (const fc::exception& e)
    {
        elog("Test completed.");
        edump((e.to_detail_string()));
        throw;
    }
}
}
}
