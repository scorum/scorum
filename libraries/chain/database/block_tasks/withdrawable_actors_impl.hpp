#pragma once

#include <scorum/chain/database/block_tasks/block_tasks.hpp>

#include <scorum/chain/schema/scorum_object_types.hpp>

namespace scorum {
namespace chain {

class account_service_i;
class dev_pool_service_i;
class dynamic_global_property_service_i;

namespace database_ns {

using scorum::protocol::asset;

// this class implements {withdrawable_id_type} specific only
class withdrawable_actors_impl
{
public:
    explicit withdrawable_actors_impl(block_task_context&);

    asset get_available_scorumpower(const withdrawable_id_type& from);

    void update_statistic(const withdrawable_id_type& from, const withdrawable_id_type& to, const asset& amount);
    void update_statistic(const withdrawable_id_type& from);

    void increase_scr(const withdrawable_id_type& id, const asset& amount);

    void increase_sp(const withdrawable_id_type& id, const asset& amount);

    void decrease_sp(const withdrawable_id_type& id, const asset& amount);

private:
    block_task_context& _ctx;
    account_service_i& _account_service;
    dev_pool_service_i& _dev_pool_service;
    dynamic_global_property_service_i& _dprops_service;
};
}
}
}
