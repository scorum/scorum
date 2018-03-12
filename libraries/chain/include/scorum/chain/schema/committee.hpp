#pragma once

#include <set>

#include <scorum/protocol/types.hpp>

#include <scorum/chain/database/database.hpp>
#include <scorum/chain/schema/registration_objects.hpp>
#include <scorum/chain/schema/dev_committee_object.hpp>

namespace scorum {
namespace chain {
namespace committee {

template <typename MemberIndexType>
std::set<account_name_type> lookup_members(const database& db, const std::string& lower_bound_name, uint32_t limit)
{
    std::set<account_name_type> ret;

    const auto& committee_members = db.get_index<MemberIndexType>().indices().template get<by_account_name>();

    for (auto it = committee_members.lower_bound(lower_bound_name); it != committee_members.end(); ++it)
    {
        ret.insert(std::cref(it->account));

        if (ret.size() >= limit)
            break;
    }

    return ret;
}

} // namespace committee
} // namespace chain
} // namespace scorum
