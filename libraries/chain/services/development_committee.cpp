#include <scorum/chain/services/development_committee.hpp>

#include <scorum/chain/schema/dev_committee_object.hpp>

namespace scorum {
namespace chain {

dbs_development_committee::dbs_development_committee(database& db)
    : _base_type(db)
{
}

void dbs_development_committee::add_member(const account_name_type& account_name)
{
    FC_ASSERT(!is_exists(account_name), "Member already exists.");
    FC_ASSERT(get_members_count() <= SCORUM_DEVELOPMENT_COMMITTEE_MAX_MEMBERS_LIMIT,
              "Can't add member. Limit ${1} is reached.", ("1", SCORUM_DEVELOPMENT_COMMITTEE_MAX_MEMBERS_LIMIT));

    db_impl().create<dev_committee_member_object>(
        [&](dev_committee_member_object& member) { member.account = account_name; });
}

void dbs_development_committee::exclude_member(const account_name_type& account_name)
{
    FC_ASSERT(get_members_count() > 1, "No committee to exclude member.");

    db_impl().remove(get_member(account_name));
}

void dbs_development_committee::change_add_member_quorum(const percent_type quorum)
{
    db_impl().modify(get(), [&](dev_committee_object& m) { m.invite_quorum = quorum; });
}

void dbs_development_committee::change_exclude_member_quorum(const percent_type quorum)
{
    db_impl().modify(get(), [&](dev_committee_object& m) { m.dropout_quorum = quorum; });
}

void dbs_development_committee::change_base_quorum(const percent_type quorum)
{
    db_impl().modify(get(), [&](dev_committee_object& m) { m.change_quorum = quorum; });
}

void dbs_development_committee::change_transfer_quorum(const percent_type quorum)
{
    db_impl().modify(get(), [&](dev_committee_object& m) { m.transfer_quorum = quorum; });
}

void dbs_development_committee::change_budgets_vcg_properties_quorum(const percent_type quorum)
{
    db_impl().modify(get(), [&](dev_committee_object& m) { m.budgets_vcg_properties_quorum = quorum; });
}

percent_type dbs_development_committee::get_add_member_quorum()
{
    return get().invite_quorum;
}

percent_type dbs_development_committee::get_exclude_member_quorum()
{
    return get().dropout_quorum;
}

percent_type dbs_development_committee::get_base_quorum()
{
    return get().change_quorum;
}

percent_type dbs_development_committee::get_transfer_quorum()
{
    return get().transfer_quorum;
}

percent_type dbs_development_committee::get_budgets_vcg_properties_quorum()
{
    return get().budgets_vcg_properties_quorum;
}

bool dbs_development_committee::is_exists(const account_name_type& account_name) const
{
    const auto& idx = db_impl().get_index<dev_committee_member_index>().indices().get<by_account_name>();
    return idx.find(account_name) != idx.cend();
}

size_t dbs_development_committee::get_members_count() const
{
    return db_impl().get_index<dev_committee_member_index>().indices().size();
}

development_committee_service_i::committee_members_cref_type dbs_development_committee::get_committee() const
{
    committee_members_cref_type ret;

    const auto& idx = db_impl().get_index<dev_committee_member_index>().indices().get<by_id>();
    for (auto it = idx.cbegin(); it != idx.cend(); ++it)
    {
        ret.push_back(std::cref(*it));
    }

    return ret;
}

const dev_committee_object& dbs_development_committee::get() const
{
    try
    {
        return db_impl().get<dev_committee_object>();
    }
    FC_CAPTURE_AND_RETHROW(("Development committee does not exist."))
}

const dev_committee_member_object& dbs_development_committee::get_member(const account_name_type& account_name) const
{
    try
    {
        return db_impl().get<dev_committee_member_object, by_account_name>(account_name);
    }
    FC_CAPTURE_AND_RETHROW((account_name))
}

} // namespace chain
} // namespace scorum
