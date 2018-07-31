#include <chrono>

namespace performance_common {
class cpu_profiler
{
public:
    cpu_profiler()
    {
        _start = std::chrono::steady_clock::now();
    }

    // milliseconds
    auto elapses() const
    {
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(now - _start).count();
    }

private:
    std::chrono::time_point<std::chrono::steady_clock> _start;
};
}
