#pragma once

#include <scorum/chain/services/service_base.hpp>
#include <scorum/chain/schema/account_objects.hpp>

namespace scorum {
namespace chain {

struct account_registration_bonus_service_i : public base_service_i<account_registration_bonus_object>
{
    virtual void remove_if_exist(const account_name_type&) = 0;

    using account_registration_bonus_refs_type = std::vector<typename base_service_i::object_cref_type>;

    virtual account_registration_bonus_refs_type get_by_expiration_time(const fc::time_point_sec& until) const = 0;
};

class dbs_account_registration_bonus : public dbs_service_base<account_registration_bonus_service_i>
{
public:
    explicit dbs_account_registration_bonus(database& db);

    void remove_if_exist(const account_name_type&) override;

    account_registration_bonus_refs_type get_by_expiration_time(const fc::time_point_sec& until) const override;
};
}
}
