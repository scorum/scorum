#pragma once

#include <scorum/chain/services/service_base.hpp>

#include <scorum/chain/schema/comment_objects.hpp>

namespace scorum {
namespace chain {

struct comments_bounty_fund_service_i : public base_service_i<comments_bounty_fund_object>
{
};

using dbs_comments_bounty_fund = dbs_service_base<comments_bounty_fund_service_i>;

} // namespace scorum
} // namespace chain
