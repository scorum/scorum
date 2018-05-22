#pragma once

#include <scorum/chain/services/service_base.hpp>

#include <scorum/protocol/block.hpp>
#include <scorum/protocol/types.hpp>

namespace scorum {
namespace chain {

using scorum::protocol::signed_block;
using scorum::protocol::block_id_type;

struct blocks_story_service_i
{
    virtual optional<signed_block> fetch_block_by_id(const block_id_type& id) const = 0;
    virtual optional<signed_block> fetch_block_by_number(uint32_t num) const = 0;
};

class dbs_blocks_story : public dbs_base, public blocks_story_service_i
{
    friend class dbservice_dbs_factory;

protected:
    explicit dbs_blocks_story(database& db);

    virtual optional<signed_block> fetch_block_by_id(const block_id_type& id) const override;
    virtual optional<signed_block> fetch_block_by_number(uint32_t num) const override;
};

} // namespace scorum
} // namespace chain
