#pragma once

#include <memory>
#include <string>
#include <typeinfo>

#include <boost/config.hpp>
#include <boost/type_index.hpp>
#include <boost/container/flat_map.hpp>

namespace scorum {
namespace chain {

class database;
class dbs_base;

class dbservice_dbs_factory
{
    using BaseServicePtr = std::unique_ptr<dbs_base>;

protected:
    dbservice_dbs_factory() = delete;

    explicit dbservice_dbs_factory(database&);

    virtual ~dbservice_dbs_factory();

public:
    template <typename ConcreteService> ConcreteService& obtain_service() const
    {
        auto it = _dbs.find(boost::typeindex::type_id<ConcreteService>());
        if (it == _dbs.end())
        {
            it = _dbs.insert(std::make_pair(boost::typeindex::type_id<ConcreteService>(),
                                            BaseServicePtr(new ConcreteService(_db_core))))
                     .first;
        }

        BaseServicePtr& ret = it->second;
        return static_cast<ConcreteService&>(*ret);
    }

    template <typename ConcreteService, typename... Args> ConcreteService& obtain_service(Args... args) const
    {
        auto it = _dbs.find(boost::typeindex::type_id<ConcreteService>());
        if (it == _dbs.end())
        {
            it = _dbs.insert(std::make_pair(boost::typeindex::type_id<ConcreteService>(),
                                            BaseServicePtr(new ConcreteService(args...))))
                     .first;
        }

        BaseServicePtr& ret = it->second;
        return static_cast<ConcreteService&>(*ret);
    }

private:
    mutable boost::container::flat_map<boost::typeindex::type_index, BaseServicePtr> _dbs;
    database& _db_core;
};
} // namespace chain
} // namespace scorum
