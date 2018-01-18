#pragma once

#include <fc/time.hpp>
#include <fc/optional.hpp>
#include <scorum/protocol/types.hpp>

namespace scorum {
namespace chain {

class database;
class dbservice_dbs_factory;

class account_service_i;
class proposal_service_i;
class committee_service_i;
class property_service_i;

class data_service_factory_i
{
public:
    virtual account_service_i& account_service() = 0;
    virtual proposal_service_i& proposal_service() = 0;
    virtual committee_service_i& committee_service() = 0;
    virtual property_service_i& property_service() = 0;
};

class data_service_factory : public data_service_factory_i
{
public:
    explicit data_service_factory(scorum::chain::database& db);

    virtual ~data_service_factory();

    virtual account_service_i& account_service();
    virtual proposal_service_i& proposal_service();
    virtual committee_service_i& committee_service();
    virtual property_service_i& property_service();

private:
    scorum::chain::dbservice_dbs_factory& factory;
};

} // namespace chain
} // namespace scorum
