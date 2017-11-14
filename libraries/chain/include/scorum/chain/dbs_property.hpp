#pragma once

#include <memory>

namespace scorum {
namespace chain {

    class dbservice;

    class dbs_property
    {
    public:
        explicit dbs_property(dbservice& db);

        typedef std::unique_ptr<dbs_property> ptr;

    public:

        //TODO

    private:

        dbservice &_db;
    };

}
}
