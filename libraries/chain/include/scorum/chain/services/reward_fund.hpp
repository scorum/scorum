#pragma once

#include <scorum/chain/services/dbs_base.hpp>

#include <scorum/chain/database/database.hpp>

#include <scorum/chain/schema/reward_objects.hpp>

namespace scorum {
namespace chain {

template <typename FundObjectType> struct reward_fund_service_i
{
    virtual bool is_exists() const = 0;

    virtual const FundObjectType& get() const = 0;

    using modifier_type = std::function<void(FundObjectType&)>;

    virtual const FundObjectType& create(const modifier_type& modifier) = 0;

    virtual void update(const modifier_type& modifier) = 0;
};

template <typename FundObjectType> class dbs_reward_fund : public dbs_base, public reward_fund_service_i<FundObjectType>
{
    friend class dbservice_dbs_factory;

protected:
    explicit dbs_reward_fund(database& db);

public:
    bool is_exists() const
    {
        return nullptr != db_impl().find<FundObjectType>();
    }

    const FundObjectType& get() const
    {
        return db_impl().get<FundObjectType>();
    }

    const FundObjectType& create(const typename reward_fund_service_i<FundObjectType>::modifier_type& modifier)
    {
        return db_impl().create<FundObjectType>([&](FundObjectType& o) { modifier(o); });
    }

    void update(const typename reward_fund_service_i<FundObjectType>::modifier_type& modifier)
    {
        db_impl().modify(get(), [&](FundObjectType& o) { modifier(o); });
    }
};

using reward_fund_scr_service_i = reward_fund_service_i<reward_fund_scr_object>;
using reward_fund_sp_service_i = reward_fund_service_i<reward_fund_sp_object>;

using dbs_reward_fund_scr = dbs_reward_fund<reward_fund_scr_object>;
using dbs_reward_fund_sp = dbs_reward_fund<reward_fund_sp_object>;

} // namespace scorum
} // namespace chain
