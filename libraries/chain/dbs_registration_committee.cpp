#include <scorum/chain/dbs_registration_committee.hpp>
#include <scorum/chain/database.hpp>

#include <scorum/chain/dbs_account.hpp>

namespace scorum {
namespace chain {

dbs_registration_committee::dbs_registration_committee(database& db)
    : _base_type(db)
{
}

bool dbs_registration_committee::is_committee_exists() const
{
    return !db_impl().get_index<registration_committee_index>().indicies().empty();
}

const registration_committee_object& dbs_registration_committee::get_committee() const
{
    auto idx = db_impl().get_index<registration_committee_index>().indicies();
    auto it = idx.cbegin();
    FC_ASSERT(it != idx.cend(), "Committee is not extsts.");
    return (*it);
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

uint64_t dbs_registration_committee::get_member_count() const
{
    return db_impl().get_index<registration_committee_member_index>().indicies().size();
}

const registration_committee_object&
dbs_registration_committee::create_committee(const genesis_state_type& genesis_state)
{
    FC_ASSERT(!is_committee_exists(), "Can't create more than one committee.");
    FC_ASSERT(!genesis_state.registration_committee.empty(), "Registration committee must have at least one member.");

    FC_ASSERT(SCORUM_REGISTRATION_LIMIT_COUNT_COMMITTEE_MEMBERS > 0, "Invalid ${1} value.",
              ("1", SCORUM_REGISTRATION_LIMIT_COUNT_COMMITTEE_MEMBERS));

    const dbs_account& account_service = db().obtain_service<dbs_account>();

    // create unique accout list form genesis unordered data
    using sorted_type = std::map<account_name_type, std::reference_wrapper<const account_object>>;
    sorted_type items;
    for (const auto& genesis_item : genesis_state.registration_committee)
    {
        const account_object& accout = account_service.get_account(genesis_item);

        items.insert(sorted_type::value_type(accout.name, std::cref(accout)));
    }

    // create empty committee
    const auto& new_pool = db_impl().create<registration_committee_object>([&](registration_committee_object&) {});

    // add members
    for (const auto& item : items)
    {
        if (get_member_count() > SCORUM_REGISTRATION_LIMIT_COUNT_COMMITTEE_MEMBERS)
        {
            wlog("Too many committee members in genesis state. More than ${1} are ignored.",
                 ("1", SCORUM_REGISTRATION_LIMIT_COUNT_COMMITTEE_MEMBERS));
            break;
        }
        const account_object& accout = item.second;
        _add_member(accout);
    }

    if (!get_member_count())
    {
        db_impl().remove(new_pool);
        FC_ASSERT(false, "Can't initialize at least one member.");
    }

    return new_pool;
}

const registration_committee_member_object&
dbs_registration_committee::add_member(const account_name_type& account_name)
{
    FC_ASSERT(!is_committee_exists(), "No committee to add memeber.");

    const dbs_account& account_service = db().obtain_service<dbs_account>();

    const account_object& accout = account_service.get_account(account_name);

    return _add_member(accout);
}

void dbs_registration_committee::exclude_member(const account_name_type& account_name)
{
    FC_ASSERT(!is_committee_exists(), "No committee to exclude memeber.");

    const dbs_account& account_service = db().obtain_service<dbs_account>();

    const account_object& accout = account_service.get_account(account_name);

    _exclude_member(accout);
}

void dbs_registration_committee::update_cash_info(const registration_committee_member_object& member,
                                                  share_type per_reg,
                                                  bool reset)
{
    FC_ASSERT(per_reg > 0, "Invalid share.");

    auto head_block_num = db_impl().head_block_num();

    db_impl().modify(member, [&](registration_committee_member_object& m) {
        m.last_allocated_block = head_block_num;
        if (reset)
        {
            m.already_allocated_cash = per_reg;
        }
        else
        {
            m.already_allocated_cash += per_reg;
        }
    });
}

bool dbs_registration_committee::_check_member_exist(const account_name_type& account_name) const
{
    const auto& idx = db_impl().get_index<registration_committee_member_index>().indices().get<by_account_name>();
    return idx.find(account_name) != idx.cend();
}

const registration_committee_member_object& dbs_registration_committee::_add_member(const account_object& account)
{
    FC_ASSERT(!_check_member_exist(account.name), "Member already exists.");
    FC_ASSERT(get_member_count() <= SCORUM_REGISTRATION_LIMIT_COUNT_COMMITTEE_MEMBERS,
              "Can't add member. Limit ${1} is reached.", ("1", SCORUM_REGISTRATION_LIMIT_COUNT_COMMITTEE_MEMBERS));

    const registration_committee_object& committee = get_committee();

    const registration_committee_member_object& new_member = db_impl().create<registration_committee_member_object>(
        [&](registration_committee_member_object& member) { member.account = account.name; });

    db_impl().modify(committee, [&](registration_committee_object& comm) { comm.members.insert(account.name); });

    return new_member;
}

void dbs_registration_committee::_exclude_member(const account_object& account)
{
    FC_ASSERT(_check_member_exist(account.name), "Member does not exist.");

    const registration_committee_object& committee = get_committee();
    const registration_committee_member_object& member = get_member(account.name);

    db_impl().modify(committee, [&](registration_committee_object& comm) { comm.members.erase(account.name); });

    db_impl().remove(member);
}
}
}
