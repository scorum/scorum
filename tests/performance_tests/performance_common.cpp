#include "performance_common.hpp"

namespace performance_common {
cpu_profiler::cpu_profiler()
{
    _start = std::chrono::steady_clock::now();
}

size_t cpu_profiler::elapsed() const
{
    auto now = std::chrono::steady_clock::now();
    return (size_t)std::chrono::duration_cast<std::chrono::milliseconds>(now - _start).count();
}
}
