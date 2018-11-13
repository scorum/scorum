#pragma once
#include <utility>

namespace scorum {
namespace chain {
namespace dba {
namespace cxx17 {
template <class...> using void_t = void;
}

template <typename TIdx, typename = cxx17::void_t<>> struct is_hashed_idx : std::false_type
{
};

template <typename TIdx> struct is_hashed_idx<TIdx, cxx17::void_t<typename std::decay_t<TIdx>::hasher>> : std::true_type
{
};
}
}
}
