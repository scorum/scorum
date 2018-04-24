#pragma once

#include <scorum/chain/services/service_base.hpp>
#include <scorum/chain/schema/comment_objects.hpp>

namespace scorum {
namespace chain {

template <class T> struct comment_statistic_base_service_i : public base_service_i<T>
{
    virtual const T& get(const comment_id_type& comment_id) const = 0;
};

template <class service_interface> class dbs_comment_statistic_base : public dbs_service_base<service_interface>
{
    friend class dbservice_dbs_factory;

    using comment_statisticobject_type = typename service_interface::object_type;

    using base_service_type = dbs_service_base<service_interface>;

protected:
    explicit dbs_comment_statistic_base(database& db)
        : base_service_type(db)
    {
    }

public:
    const comment_statisticobject_type& get(const comment_id_type& comment_id) const override
    {
        try
        {
            return base_service_type::template get_by<by_comment_id>(comment_id);
        }
        FC_CAPTURE_AND_RETHROW((comment_id))
    }
};

struct comment_statistic_scr_service_i : public comment_statistic_base_service_i<comment_statistic_scr_object>
{
};

using dbs_comment_statistic_scr = dbs_comment_statistic_base<comment_statistic_scr_service_i>;

struct comment_statistic_sp_service_i : public comment_statistic_base_service_i<comment_statistic_sp_object>
{
};

using dbs_comment_statistic_sp = dbs_comment_statistic_base<comment_statistic_sp_service_i>;

} // namespace chain
} // namespace scorum
