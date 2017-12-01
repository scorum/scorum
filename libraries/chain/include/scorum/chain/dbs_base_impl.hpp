#pragma once

#include <memory>
#include <map>
#include <string>
#include <typeinfo>

#include <boost/config.hpp>

#include <scorum/chain/dbservice_common.hpp>

namespace scorum {
namespace chain {

class dbservice;
class database;

struct dbs_base
{
protected:
    dbs_base() = delete;
    dbs_base(dbs_base &&) = delete;

    explicit dbs_base(database&);

    typedef dbs_base _base_type;

public:
    virtual ~dbs_base();

    dbservice& db();

protected:
    database& db_impl();
    const database& db_impl() const;

    time_point_sec _get_now(const optional<time_point_sec>& = optional<time_point_sec>());

    typedef time_point_sec _time;

private:
    database& _db_core;
};

class dbservice_dbs_factory
{
    typedef std::unique_ptr<dbs_base> BaseServicePtr;

protected:
    dbservice_dbs_factory() = delete;

    explicit dbservice_dbs_factory(database&);

    virtual ~dbservice_dbs_factory();

public:
    template <typename ConcreteService> ConcreteService& obtain_service() const
    {
        auto it = _dbs.find(typeid(ConcreteService).name());
        if (it == _dbs.end())
        {
            it = _dbs.insert(std::pair<std::string, BaseServicePtr>(typeid(ConcreteService).name(),
                                                                    BaseServicePtr(new ConcreteService(_db_core))))
                     .first;
        }

        BaseServicePtr& ret = it->second;
        return static_cast<ConcreteService&>(*ret);
    }

private:
    mutable std::map<std::string, BaseServicePtr> _dbs;
    database& _db_core;
};
}
}
