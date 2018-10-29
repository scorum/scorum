#include <scorum/chain/services/blocks_story.hpp>

#include <scorum/chain/database/database.hpp>

using namespace scorum::protocol;

namespace scorum {
namespace chain {

dbs_blocks_story::dbs_blocks_story(database& db)
    : dbs_base(db)
    , _db(db)
{
}

optional<signed_block> dbs_blocks_story::fetch_block_by_id(const block_id_type& id) const
{
    return _db.fetch_block_by_id(id);
}

optional<signed_block> dbs_blocks_story::fetch_block_by_number(uint32_t num) const
{
    return _db.fetch_block_by_number(num);
}
}
}
