#pragma once

#include <scorum/chain/services/service_base.hpp>
#include <scorum/chain/schema/comment_objects.hpp>

namespace scorum {
namespace chain {

struct comment_statistic_service_i : public base_service_i<comment_statistic_object>
{
    virtual const comment_statistic_object& get(const comment_id_type& comment_id) const = 0;
};

class dbs_comment_statistic : public dbs_service_base<comment_statistic_service_i>
{
    friend class dbservice_dbs_factory;

protected:
    explicit dbs_comment_statistic(database& db);

public:
    const comment_statistic_object& get(const comment_id_type& comment_id) const override;
};

} // namespace chain
} // namespace scorum
