#pragma once

#include <memory>
#include <map>
#include <string>
#include <typeinfo>

#include <boost/config.hpp>
#include <boost/type_index.hpp>

#include <scorum/chain/scorum_object_types.hpp>
#include <scorum/protocol/asset.hpp>

namespace scorum {
namespace chain {

class dbservice_dbs_factory;
class database;

using scorum::protocol::authority;
using scorum::protocol::account_authority_map;
using scorum::protocol::asset;
using scorum::protocol::asset_symbol_type;
using scorum::protocol::price;
using scorum::protocol::public_key_type;

struct dbs_base
{
protected:
    dbs_base() = delete;
    dbs_base(dbs_base&&) = delete;

    explicit dbs_base(database&);

    typedef dbs_base _base_type;

public:
    virtual ~dbs_base();

    dbservice_dbs_factory& db();

    time_point_sec head_block_time();

protected:
    database& db_impl();
    const database& db_impl() const;

private:
    database& _db_core;
};

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

private:
    mutable std::map<boost::typeindex::type_index, BaseServicePtr> _dbs;
    database& _db_core;
};
} // namespace chain
} // namespace scorum
