#pragma once

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/string_generator.hpp>
#include <boost/uuid/name_generator.hpp>

namespace scorum {
namespace chain {
struct bet_data;
}
}

boost::uuids::uuid gen_uuid(const std::string& seed);

bool compare_bet_data(const scorum::chain::bet_data& lhs, const scorum::chain::bet_data& rhs);
