#pragma once

#include <memory>

namespace scorum {
namespace chain {

    class dbservice;
    class database;

    class dbs_account
    {
    public:
        explicit dbs_account(dbservice& db);

        typedef std::unique_ptr<dbs_account> ptr;

    public:

        //TODO

    private:

        database &_db;
    };

}
}
