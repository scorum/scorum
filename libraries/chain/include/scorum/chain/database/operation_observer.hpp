#pragma once
#include <functional>
#include <fc/signals.hpp>

#include <scorum/chain/tasks_base.hpp>
#include <scorum/chain/operation_notification.hpp>
#include <scorum/protocol/proposal_operations.hpp>

namespace scorum {
namespace chain {
namespace database_ns {

template <typename TOp> class operation_observer
{
public:
    using result_type = void;

    operation_observer(fc::signal<void(const operation_notification&)>& op_signal,
                       std::function<void(const TOp&)> handler)
        : _handler(std::move(handler))
    {
        _conn = op_signal.connect([&](const operation_notification& note) { note.op.visit(*this); });
    }
    operation_observer(fc::signal<void(const protocol::proposal_operation&)>& op_signal,
                       std::function<void(const TOp&)> handler)
        : _handler(std::move(handler))
    {
        _conn = op_signal.connect([&](const protocol::proposal_operation& op) { op.visit(*this); });
    }

    void operator()(const TOp& op)
    {
        _handler(op);
    }

    template <typename Op> void operator()(Op&&) const
    {
    } /// ignore all other ops

private:
    fc::scoped_connection _conn;
    std::function<void(const TOp&)> _handler;
};
}
}
}
