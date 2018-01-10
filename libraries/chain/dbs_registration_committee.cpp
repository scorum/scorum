#include <scorum/chain/dbs_registration_committee.hpp>
#include <scorum/chain/database.hpp>

#include <scorum/chain/dbs_account.hpp>

namespace scorum {
namespace chain {

dbs_registration_committee::dbs_registration_committee(database& db)
    : _base_type(db)
{
}

dbs_registration_committee::registration_committee_member_refs_type dbs_registration_committee::get_committee() const
{
    registration_committee_member_refs_type ret;

    const auto& idx = db_impl().get_index<registration_committee_member_index>().indices();
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
        if (_get_member_count() > SCORUM_REGISTRATION_LIMIT_COUNT_COMMITTEE_MEMBERS)
        {
            wlog("Too many committee members in genesis state. More than ${1} are ignored.",
                 ("1", SCORUM_REGISTRATION_LIMIT_COUNT_COMMITTEE_MEMBERS));
            break;
        }
        const account_object& accout = item.second;
        _add_member(accout);
    }

    if (!_get_member_count())
    {
        FC_ASSERT(false, "Can't initialize at least one member.");
    }

    return get_committee();
}

const registration_committee_member_object&
dbs_registration_committee::add_member(const account_name_type& account_name)
{
    // to fill empty committee it is used create_committee
    FC_ASSERT(!get_committee().empty(), "No committee to add member.");

    const dbs_account& account_service = db().obtain_service<dbs_account>();

    const account_object& accout = account_service.get_account(account_name);

    return _add_member(accout);
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
                                                    member_info_modifier_type modifier)
{
    db_impl().modify(member, [&](registration_committee_member_object& m) { modifier(m); });
}

uint64_t dbs_registration_committee::_get_member_count() const
{
    return db_impl().get_index<registration_committee_member_index>().indices().size();
}

bool dbs_registration_committee::member_exists(const account_name_type& account_name) const
{
    const auto& idx = db_impl().get_index<registration_committee_member_index>().indices().get<by_account_name>();
    return idx.find(account_name) != idx.cend();
}

const registration_committee_member_object& dbs_registration_committee::_add_member(const account_object& account)
{
    FC_ASSERT(!member_exists(account.name), "Member already exists.");
    FC_ASSERT(_get_member_count() <= SCORUM_REGISTRATION_LIMIT_COUNT_COMMITTEE_MEMBERS,
              "Can't add member. Limit ${1} is reached.", ("1", SCORUM_REGISTRATION_LIMIT_COUNT_COMMITTEE_MEMBERS));

    const registration_committee_member_object& new_member = db_impl().create<registration_committee_member_object>(
        [&](registration_committee_member_object& member) { member.account = account.name; });

    return new_member;
}

void dbs_registration_committee::_exclude_member(const account_object& account)
{
    FC_ASSERT(member_exists(account.name), "Member does not exist.");

    const registration_committee_member_object& member = get_member(account.name);

    db_impl().remove(member);
}
} // namespace chain
} // namespace scorum
