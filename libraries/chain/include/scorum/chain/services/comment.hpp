#pragma once

#include <scorum/chain/services/dbs_base.hpp>

#include <functional>

namespace scorum {
namespace chain {

class comment_object;

struct comment_service_i
{
    virtual const comment_object& get(const comment_id_type& comment_id) const = 0;
    virtual const comment_object& get(const account_name_type& author, const std::string& permlink) const = 0;

    virtual bool is_exists(const account_name_type& author, const std::string& permlink) const = 0;

    using modifier_type = std::function<void(comment_object&)>;

    virtual const comment_object& create(const modifier_type& modifier) = 0;

    virtual void update(const comment_object& comment, const modifier_type& modifier) = 0;

    virtual void remove(const comment_object& comment) = 0;
};

class dbs_comment : public dbs_base, public comment_service_i
{
    friend class dbservice_dbs_factory;

protected:
    explicit dbs_comment(database& db);

public:
    const comment_object& get(const comment_id_type& comment_id) const override;
    const comment_object& get(const account_name_type& author, const std::string& permlink) const override;

    bool is_exists(const account_name_type& author, const std::string& permlink) const override;

    const comment_object& create(const modifier_type& modifier) override;

    void update(const comment_object& comment, const modifier_type& modifier) override;

    void remove(const comment_object& comment) override;
};
} // namespace chain
} // namespace scorum
