#include <db_mock.hpp>

//#include <fc/log/logger.hpp>
//#include <fc/log/log_message.hpp>

db_mock::db_mock(uint32_t file_size)
    : _path(boost::filesystem::unique_path())
{
    FC_ASSERT(!boost::filesystem::exists(_path));

    //    auto loger_cfg = fc::logger::get(DEFAULT_LOGGER);
    chainbase::database::open(_path, chainbase::database::read_write, file_size);
}

db_mock::~db_mock()
{
    boost::system::error_code code;
    boost::filesystem::remove_all(_path, code);
}
