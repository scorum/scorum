#pragma once

#include <scorum/chain/services/dbs_base.hpp>

#include <scorum/chain/database/database.hpp>

#include <scorum/chain/schema/reward_objects.hpp>

#include <functional>

namespace scorum {
namespace chain {

template <typename FundObjectType> struct reward_fund_service_i
{
    using object_type = FundObjectType;
    using modifier_type = std::function<void(object_type&)>;

    virtual bool is_exists() const = 0;

    virtual const FundObjectType& get() const = 0;

    virtual const FundObjectType& create(const modifier_type& modifier) = 0;

    virtual void update(const modifier_type& modifier) = 0;
};

template <typename InterfaceType> class dbs_reward_fund : public dbs_base, public InterfaceType
{
protected:
    dbs_reward_fund::dbs_reward_fund(database& db)
        : dbs_base(db)
    {
    }

    using base_service_type = dbs_reward_fund;

public:
    using modifier_type = typename InterfaceType::modifier_type;
    using object_type = typename InterfaceType::object_type;

    virtual bool is_exists() const
    {
        return nullptr != db_impl().template find<object_type>();
    }

    virtual const object_type& get() const
    {
        return db_impl().template get<object_type>();
    }

    virtual const object_type& create(const modifier_type& modifier)
    {
        return db_impl().template create<object_type>([&](object_type& o) { modifier(o); });
    }

    virtual void update(const modifier_type& modifier)
    {
        db_impl().template modify(get(), [&](object_type& o) { modifier(o); });
    }
};

struct reward_fund_scr_service_i : public reward_fund_service_i<reward_fund_scr_object>
{
};
struct reward_fund_sp_service_i : public reward_fund_service_i<reward_fund_sp_object>
{
};

struct dbs_reward_fund_scr : public dbs_reward_fund<reward_fund_scr_service_i>
{
    friend class dbservice_dbs_factory;

protected:
    explicit dbs_reward_fund_scr(database& db);
};

struct dbs_reward_fund_sp : public dbs_reward_fund<reward_fund_sp_service_i>
{
    friend class dbservice_dbs_factory;

protected:
    explicit dbs_reward_fund_sp(database& db);
};

} // namespace scorum
} // namespace chain
