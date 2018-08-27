#pragma once
#include <boost/range/any_range.hpp>

namespace scorum {
namespace utils {
template <typename TObject> using forward_range = boost::any_range<TObject, boost::forward_traversal_tag>;
}
}
