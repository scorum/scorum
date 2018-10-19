#pragma once

#include <chainbase/chainbase.hpp>

#include "logger.hpp"

class db_mock : public chainbase::database
{
public:
    db_mock();
    virtual ~db_mock();

    void open();

private:
    const boost::filesystem::path _path;
};
