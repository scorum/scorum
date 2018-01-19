#include <scorum/chain/data_service_factory.hpp>

#include <scorum/chain/database.hpp>

#include <scorum/chain/dbs_account.hpp>
#include <scorum/chain/dbs_proposal.hpp>
#include <scorum/chain/dbs_registration_committee.hpp>
#include <scorum/chain/dbs_dynamic_global_property.hpp>
#include <scorum/chain/dbs_comment.hpp>
#include <scorum/chain/dbs_decline_voting_rights_request.hpp>
#include <scorum/chain/dbs_witness_vote.hpp>

// clang-format off
DATA_SERVICE_FACTORY_IMPL(
        (account)
        (proposal)
        (registration_committee)
        (dynamic_global_property)
        (comment)
        (decline_voting_rights_request)
        (witness_vote)
        )
// clang-format on
