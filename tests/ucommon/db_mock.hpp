#include <chainbase/chainbase.hpp>
#include <boost/filesystem/path.hpp>

class db_mock : public chainbase::database
{
public:
    db_mock(uint32_t file_size = 10'000'000);
    ~db_mock();

private:
    const boost::filesystem::path _path;
};
