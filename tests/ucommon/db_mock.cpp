#include "db_mock.hpp"

db_mock::db_mock()
    : _path(boost::filesystem::unique_path())
{
    FC_ASSERT(!boost::filesystem::exists(_path));
}

db_mock::~db_mock()
{
    close();

    boost::system::error_code code;
    boost::filesystem::remove_all(_path, code);
}

void db_mock::open()
{
    tests::initialize_logger(fc::log_level::error);
    chainbase::database::open(_path, chainbase::database::read_write, 10000000);
    tests::initialize_logger(fc::log_level::info);
}
