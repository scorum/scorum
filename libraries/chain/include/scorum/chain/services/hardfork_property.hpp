#pragma once

#include <scorum/chain/services/service_base.hpp>
#include <scorum/chain/hardfork.hpp>

namespace scorum {
namespace chain {

struct hardfork_property_service_i : public base_service_i<hardfork_property_object>
{
    virtual bool has_hardfork(uint32_t hardfork) const = 0;
};

class dbs_hardfork_property : public dbs_service_base<hardfork_property_service_i>
{
    friend class dbservice_dbs_factory;

protected:
    explicit dbs_hardfork_property(database& db);

public:
    virtual bool has_hardfork(uint32_t hardfork) const override;
};

} // namespace chain
} // namespace scorum
