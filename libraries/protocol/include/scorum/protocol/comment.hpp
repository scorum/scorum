#pragma once
#include <scorum/protocol/asset.hpp>
#include <scorum/protocol/config.hpp>

namespace scorum {
namespace protocol {

struct beneficiary_route_type
{
    beneficiary_route_type()
    {
    }
    beneficiary_route_type(const account_name_type& a, const uint16_t& w)
        : account(a)
        , weight(w)
    {
    }

    account_name_type account;
    uint16_t weight;

    // For use by std::sort such that the route is sorted first by name (ascending)
    bool operator<(const beneficiary_route_type& o) const
    {
        return account < o.account;
    }
};

struct comment_payout_beneficiaries
{
    std::vector<beneficiary_route_type> beneficiaries;

    void validate() const;
};

typedef static_variant<comment_payout_beneficiaries> comment_options_extension;

typedef flat_set<comment_options_extension> comment_options_extensions_type;
}
}

FC_REFLECT(scorum::protocol::beneficiary_route_type, (account)(weight))
FC_REFLECT(scorum::protocol::comment_payout_beneficiaries, (beneficiaries))
FC_REFLECT_TYPENAME(scorum::protocol::comment_options_extension)
