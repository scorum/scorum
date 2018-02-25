#include <scorum/chain/services/registration_committee.hpp>
#include <scorum/chain/database.hpp>

#include <scorum/chain/services/account.hpp>

#include <scorum/chain/schema/registration_objects.hpp>
#include <scorum/chain/schema/account_objects.hpp>

namespace scorum {
namespace chain {

dbs_registration_committee::dbs_registration_committee(database& db)
    : _base_type(db)
{
}

dbs_registration_committee::registration_committee_member_refs_type dbs_registration_committee::get_committee() const
{
    registration_committee_member_refs_type ret;

    const auto& idx = db_impl().get_index<registration_committee_member_index>().indices().get<by_id>();
    for (auto it = idx.cbegin(); it != idx.cend(); ++it)
    {
        ret.push_back(std::cref(*it));
    }

    return ret;
}

const registration_committee_member_object&
dbs_registration_committee::get_member(const account_name_type& account) const
{
    try
    {
        return db_impl().get<registration_committee_member_object, by_account_name>(account);
    }
    FC_CAPTURE_AND_RETHROW((account))
}

dbs_registration_committee::registration_committee_member_refs_type
dbs_registration_committee::create_committee(const std::vector<account_name_type>& accounts)
{
    FC_ASSERT(!accounts.empty(), "Registration committee must have at least one member.");
    FC_ASSERT(SCORUM_REGISTRATION_LIMIT_COUNT_COMMITTEE_MEMBERS > 0, "Invalid ${1} value.",
              ("1", SCORUM_REGISTRATION_LIMIT_COUNT_COMMITTEE_MEMBERS));

    // check existence here to allow unit tests check input data even if object exists in DB
    FC_ASSERT(get_committee().empty(), "Can't create more than one committee.");

    const dbs_account& account_service = db().obtain_service<dbs_account>();

    // create unique accout list form genesis unordered data
    using sorted_type = std::map<account_name_type, std::reference_wrapper<const account_object>>;
    sorted_type items;
    for (const auto& account_name : accounts)
    {
        const account_object& accout = account_service.get_account(account_name);

        items.insert(sorted_type::value_type(accout.name, std::cref(accout)));
    }

    // add members
    for (const auto& item : items)
    {
        if (get_members_count() > SCORUM_REGISTRATION_LIMIT_COUNT_COMMITTEE_MEMBERS)
        {
            wlog("Too many committee members in genesis state. More than ${1} are ignored.",
                 ("1", SCORUM_REGISTRATION_LIMIT_COUNT_COMMITTEE_MEMBERS));
            break;
        }
        const account_object& accout = item.second;
        _add_member(accout);
    }

    if (!get_members_count())
    {
        FC_ASSERT(false, "Can't initialize at least one member.");
    }

    return get_committee();
}

void dbs_registration_committee::add_member(const account_name_type& account_name)
{
    // to fill empty committee it is used create_committee
    FC_ASSERT(!get_committee().empty(), "No committee to add member.");

    const dbs_account& account_service = db().obtain_service<dbs_account>();

    const account_object& accout = account_service.get_account(account_name);

    _add_member(accout);
}

void dbs_registration_committee::exclude_member(const account_name_type& account_name)
{
    // committee must exist after exclude (at least one member)
    FC_ASSERT(get_committee().size() > 1, "No committee to exclude member.");

    const dbs_account& account_service = db().obtain_service<dbs_account>();

    const account_object& accout = account_service.get_account(account_name);

    _exclude_member(accout);
}

void dbs_registration_committee::update_member_info(const registration_committee_member_object& member,
                                                    const member_info_modifier_type& modifier)
{
    db_impl().modify(member, [&](registration_committee_member_object& m) { modifier(m); });
}

uint64_t dbs_registration_committee::get_members_count() const
{
    return db_impl().get_index<registration_committee_member_index>().indices().size();
}

bool dbs_registration_committee::is_exists(const account_name_type& account_name) const
{
    const auto& idx = db_impl().get_index<registration_committee_member_index>().indices().get<by_account_name>();
    return idx.find(account_name) != idx.cend();
}

void dbs_registration_committee::change_add_member_quorum(const protocol::percent_type quorum)
{
}

void dbs_registration_committee::change_exclude_member_quorum(const percent_type quorum)
{
}

void dbs_registration_committee::change_base_quorum(const percent_type quorum)
{
}

percent_type dbs_registration_committee::get_add_member_quorum()
{
    return SCORUM_COMMITTEE_QUORUM_PERCENT;
}

percent_type dbs_registration_committee::get_exclude_member_quorum()
{
    return SCORUM_COMMITTEE_QUORUM_PERCENT;
}

percent_type dbs_registration_committee::get_base_quorum()
{
    return SCORUM_COMMITTEE_QUORUM_PERCENT;
}

const registration_committee_member_object& dbs_registration_committee::_add_member(const account_object& account)
{
    FC_ASSERT(!is_exists(account.name), "Member already exists.");
    FC_ASSERT(get_members_count() <= SCORUM_REGISTRATION_LIMIT_COUNT_COMMITTEE_MEMBERS,
              "Can't add member. Limit ${1} is reached.", ("1", SCORUM_REGISTRATION_LIMIT_COUNT_COMMITTEE_MEMBERS));

    const registration_committee_member_object& new_member = db_impl().create<registration_committee_member_object>(
        [&](registration_committee_member_object& member) { member.account = account.name; });

    return new_member;
}

void dbs_registration_committee::_exclude_member(const account_object& account)
{
    FC_ASSERT(is_exists(account.name), "Member does not exist.");

    const registration_committee_member_object& member = get_member(account.name);

    db_impl().remove(member);
}

namespace utils {

bool is_quorum(size_t votes, size_t members_count, size_t quorum)
{
    const size_t voted_percent = votes * SCORUM_100_PERCENT / members_count;

    return voted_percent >= SCORUM_PERCENT(quorum) ? true : false;
}

} // namespace utils

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

} // namespace chain
} // namespace scorum
