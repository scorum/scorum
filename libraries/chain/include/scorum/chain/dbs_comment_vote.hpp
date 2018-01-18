#pragma once

#include <scorum/chain/dbs_base_impl.hpp>

#include <scorum/chain/comment_object.hpp>

#include <functional>

namespace scorum {
namespace chain {

class dbs_comment_vote : public dbs_base
{
    friend class dbservice_dbs_factory;

protected:
    explicit dbs_comment_vote(database& db);

public:
    const comment_vote_object& get(const comment_id_type& comment_id, const account_id_type& voter_id) const;

    bool is_exists(const comment_id_type& comment_id, const account_id_type& voter_id) const;

    using modifier_type = std::function<void(comment_vote_object&)>;

    const comment_vote_object& create(modifier_type modifier);

    void update(const comment_vote_object& comment_vote, modifier_type modifier);

    void remove(const comment_id_type& comment_id);
};
} // namespace chain
} // namespace scorum
