#pragma once

#include <fc/time.hpp>
#include <fc/optional.hpp>
#include <scorum/protocol/types.hpp>

namespace scorum {
namespace chain {

namespace sp = scorum::protocol;

class proposal_object;

class account_service_i
{
public:
    //    virtual void check_account_existence(const sp::account_name_type&) const = 0;

    virtual void check_account_existence(const sp::account_name_type&,
                                         const fc::optional<const char*>& context_type_name
                                         = fc::optional<const char*>()) const;
};

class proposal_service_i
{
public:
    virtual void create(const sp::account_name_type& creator,
                        const fc::variant& data,
                        sp::proposal_action action,
                        const fc::time_point_sec& expiration,
                        uint64_t quorum)
        = 0;
};

class committee_service_i
{
public:
    virtual bool member_exists(const sp::account_name_type&) const = 0;
};

class property_service_i
{
public:
};

class data_service_factory_i
{
public:
    virtual fc::time_point_sec head_block_time() = 0;

    virtual account_service_i& account_service() = 0;
    virtual proposal_service_i& proposal_service() = 0;
    virtual committee_service_i& committee_service() = 0;
    virtual property_service_i& property_service() = 0;
};

class data_service_factory
{
public:
};

} // namespace chain
} // namespace scorum
