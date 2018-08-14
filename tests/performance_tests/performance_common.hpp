#include <chrono>
#include <algorithm>

namespace performance_common {
class cpu_profiler
{
public:
    cpu_profiler();

    // milliseconds
    size_t elapsed() const;

private:
    std::chrono::time_point<std::chrono::steady_clock> _start;
};
}
