#pragma once

#include <memory>
#include <map>
#include <string>
#include <typeinfo>

#include <boost/config.hpp>
#include <boost/type_index.hpp>

#include <scorum/chain/schema/scorum_object_types.hpp>
#include <scorum/protocol/asset.hpp>

#include <scorum/chain/database.hpp>

namespace scorum {
namespace chain {

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

} // namespace chain
} // namespace scorum
