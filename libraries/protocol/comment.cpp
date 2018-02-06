#include <scorum/protocol/comment.hpp>
#include <scorum/protocol/scorum_operations.hpp>

namespace scorum {
namespace protocol {

void comment_payout_beneficiaries::validate() const
{
    uint32_t sum = 0;

    FC_ASSERT(beneficiaries.size(), "Must specify at least one beneficiary");
    FC_ASSERT(beneficiaries.size() < 128,
              "Cannot specify more than 127 beneficiaries."); // Require size serialization fits in one byte.

    validate_account_name(beneficiaries[0].account);
    FC_ASSERT(beneficiaries[0].weight <= SCORUM_100_PERCENT,
              "Cannot allocate more than 100% of rewards to one account");
    sum += beneficiaries[0].weight;
    FC_ASSERT(
        sum <= SCORUM_100_PERCENT,
        "Cannot allocate more than 100% of rewards to a comment"); // Have to check incrementally to avoid overflow

    for (size_t i = 1; i < beneficiaries.size(); i++)
    {
        validate_account_name(beneficiaries[i].account);
        FC_ASSERT(beneficiaries[i].weight <= SCORUM_100_PERCENT,
                  "Cannot allocate more than 100% of rewards to one account");
        sum += beneficiaries[i].weight;
        FC_ASSERT(
            sum <= SCORUM_100_PERCENT,
            "Cannot allocate more than 100% of rewards to a comment"); // Have to check incrementally to avoid overflow
        FC_ASSERT(beneficiaries[i - 1] < beneficiaries[i],
                  "Benficiaries must be specified in sorted order (account ascending)");
    }
}
}
}
