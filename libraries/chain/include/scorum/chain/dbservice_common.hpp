#pragma once

#include <scorum/chain/global_property_object.hpp>
#include <scorum/chain/node_property_object.hpp>
#include <scorum/chain/custom_operation_interpreter.hpp>

namespace scorum {
namespace chain {

class dbservice;
class database;

using scorum::protocol::authority;
using scorum::protocol::asset;
using scorum::protocol::asset_symbol_type;
using scorum::protocol::price;
using scorum::protocol::public_key_type;

using fc::time_point_sec;

class witness_object;
class account_object;
class comment_object;
class escrow_object;
class shared_authority;

}
}
