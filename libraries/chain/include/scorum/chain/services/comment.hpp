#pragma once

#include <scorum/chain/services/service_base.hpp>
#include <scorum/chain/schema/comment_objects.hpp>

namespace scorum {
namespace chain {

struct comment_service_i : public base_service_i<comment_object>
{
    virtual const comment_object& get(const comment_id_type& comment_id) const = 0;
    virtual const comment_object& get(const account_name_type& author, const std::string& permlink) const = 0;

    using comment_refs_type = std::vector<std::reference_wrapper<const comment_object>>;

    using checker_type = std::function<bool(const comment_object&)>;

    virtual comment_refs_type get_by_cashout_time(const checker_type& until) const = 0;

    virtual comment_refs_type get_by_cashout_time(const checker_type& until, const checker_type& filter) const = 0;

    virtual bool is_exists(const account_name_type& author, const std::string& permlink) const = 0;
};

class dbs_comment : public dbs_service_base<comment_service_i>
{
    friend class dbservice_dbs_factory;

protected:
    explicit dbs_comment(database& db);

public:
    const comment_object& get(const comment_id_type& comment_id) const override;
    const comment_object& get(const account_name_type& author, const std::string& permlink) const override;

    comment_refs_type get_by_cashout_time(const checker_type& until) const override;

    comment_refs_type get_by_cashout_time(const checker_type& until, const checker_type& filter) const override;

    bool is_exists(const account_name_type& author, const std::string& permlink) const override;
};

} // namespace chain
} // namespace scorum
