#include <scorum/chain/services/development_committee.hpp>

#include <scorum/chain/schema/dev_committee_object.hpp>

namespace scorum {
namespace chain {

dbs_development_committee::dbs_development_committee(database& db)
    : _base_type(db)
{
}

void dbs_development_committee::add_member(const account_name_type&)
{
}

void dbs_development_committee::exclude_member(const account_name_type&)
{
}

void dbs_development_committee::change_add_member_quorum(const percent_type quorum)
{
}

void dbs_development_committee::change_exclude_member_quorum(const percent_type quorum)
{
}

void dbs_development_committee::change_base_quorum(const percent_type quorum)
{
}

percent_type dbs_development_committee::get_add_member_quorum()
{
    return SCORUM_COMMITTEE_QUORUM_PERCENT;
}

percent_type dbs_development_committee::get_exclude_member_quorum()
{
    return SCORUM_COMMITTEE_QUORUM_PERCENT;
}

percent_type dbs_development_committee::get_base_quorum()
{
    return SCORUM_COMMITTEE_QUORUM_PERCENT;
}

bool dbs_development_committee::is_exists(const account_name_type&) const
{
    return true;
}

size_t dbs_development_committee::get_members_count() const
{
    return 0;
}

development_committee_service_i::member_object_cref_type dbs_development_committee::get_committee() const
{
    member_object_cref_type ret;

    const auto& idx = db_impl().get_index<dev_committee_member_index>().indices().get<by_id>();
    for (auto it = idx.cbegin(); it != idx.cend(); ++it)
    {
        ret.push_back(std::cref(*it));
    }

    return ret;
}

} // namespace chain
} // namespace scorum
