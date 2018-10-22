#pragma once

#include <scorum/protocol/types.hpp>
#include <scorum/chain/services/dbs_base.hpp>
#include <scorum/chain/schema/comment_objects.hpp>

namespace scorum {

namespace chain {
class database;
}

namespace tags {

using scorum::protocol::asset;
using scorum::chain::comment_object;
using scorum::chain::comment_id_type;

class tag_object;

struct tags_service_i
{
    virtual void set_promoted_balance(comment_id_type id, asset& promoted) const = 0;
    virtual const time_point_sec calculate_discussion_payout_time(const comment_object& comment) const = 0;
};

class tags_service : public scorum::chain::dbs_base, public tags_service_i
{
    friend class chain::dbservice_dbs_factory;

public:
    explicit tags_service(scorum::chain::database& db);

    void set_promoted_balance(comment_id_type id, asset& promoted) const override;
    const time_point_sec calculate_discussion_payout_time(const comment_object& comment) const override;
};

} // namespace tags
} // namespace scorum
