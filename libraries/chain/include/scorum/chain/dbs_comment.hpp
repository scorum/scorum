#pragma once

#include <scorum/chain/dbs_base_impl.hpp>

#include <scorum/chain/comment_object.hpp>

#include <functional>

namespace scorum {
namespace chain {

class dbs_comment : public dbs_base
{
    friend class dbservice_dbs_factory;

protected:
    explicit dbs_comment(database& db);

public:
    const comment_object& get(const comment_id_type& comment_id) const;
    const comment_object& get(const account_name_type& author, const std::string& permlink) const;

    bool is_exists(const account_name_type& author, const std::string& permlink) const;

    using modifier_type = std::function<void(comment_object&)>;

    const comment_object& create(modifier_type modifier);

    void update(const comment_object& comment, modifier_type modifier);

    void remove(const comment_object& comment);
};
} // namespace chain
} // namespace scorum
