#pragma once

#include <scorum/chain/services/dbs_base.hpp>

// This is the new none dbs-service
// Look BLOC-415 and refactor this class
// ===========================================
// It must not inherit dbs_base,
// replace dbs_betting to betting_service_impl

namespace scorum {
namespace chain {
struct betting_service_i
{
    virtual bool is_betting_moderator(const account_name_type& account_name) const = 0;
};

struct betting_property_service_i;

class dbs_betting : public dbs_base, public betting_service_i
{
    friend class dbservice_dbs_factory;

protected:
    dbs_betting(database& db);

public:
    virtual bool is_betting_moderator(const account_name_type& account_name) const override;

private:
    betting_property_service_i& _betting_property;
};
}
}
