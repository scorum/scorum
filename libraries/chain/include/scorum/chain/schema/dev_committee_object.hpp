#pragma once

#include <scorum/protocol/asset.hpp>

#include <scorum/chain/schema/scorum_object_types.hpp>

namespace scorum {
namespace chain {

using scorum::protocol::asset;

class dev_committee : public object<dev_committee_object_type, dev_committee>
{
public:
    CHAINBASE_DEFAULT_CONSTRUCTOR(dev_committee)

    id_type id;

    asset balance = asset(0, SCORUM_SYMBOL);
};

typedef shared_multi_index_container<dev_committee,
                                     indexed_by<ordered_unique<tag<by_id>,
                                                               member<dev_committee,
                                                                      dev_committee::id_type,
                                                                      &dev_committee::id>>>>
    dev_committee_index;

} // namespace chain
} // namespace scorum

FC_REFLECT(scorum::chain::dev_committee, (id)(balance))
CHAINBASE_SET_INDEX_TYPE(scorum::chain::dev_committee, scorum::chain::dev_committee_index)
