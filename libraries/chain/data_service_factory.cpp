#include <scorum/chain/data_service_factory.hpp>

#include <scorum/chain/database.hpp>

#include <scorum/chain/dbs_account.hpp>
#include <scorum/chain/dbs_proposal.hpp>
#include <scorum/chain/dbs_registration_committee.hpp>
#include <scorum/chain/dbs_dynamic_global_property.hpp>

namespace scorum {
namespace chain {

data_service_factory::data_service_factory(scorum::chain::database& db)
    : factory(db)
{
}

data_service_factory::~data_service_factory()
{
}

account_service_i& data_service_factory::account_service()
{
    return factory.obtain_service<dbs_account>();
}

proposal_service_i& data_service_factory::proposal_service()
{
    return factory.obtain_service<dbs_proposal>();
}

committee_service_i& data_service_factory::committee_service()
{
    return factory.obtain_service<dbs_registration_committee>();
}

property_service_i& data_service_factory::property_service()
{
    return factory.obtain_service<dbs_dynamic_global_property>();
}

} // namespace chain
} // namespace scorum
