#pragma once

#include <chainbase/abstract_interfaces.hpp>

namespace chainbase {

class session : public abstract_undo_session
{
    // SM description
    // clang-format off
    struct empty_state
    {
        virtual ~empty_state() {}
        virtual void process_undo(session&ctx) {}
        virtual void process_push(session&ctx) {}
    };
    // clang-format on

    struct undo_state : public empty_state
    {
        virtual void process_undo(session& ctx)
        {
            ctx._index.undo();
            ctx.transit2<empty_state>();
        }
        virtual void process_push(session& ctx)
        {
            ctx.transit2<empty_state>();
        }
    };

    template <typename new_state> void transit2()
    {
        static new_state s_state;
        _state = &s_state;
    }

public:
    session(abstract_generic_index_i& idx)
        : _index(idx)
    {
        transit2<undo_state>();
    }

    ~session()
    {
        _state->process_undo(*this);
    }

    /** leaves the UNDO state on the stack when session goes out of scope */
    void push() override
    {
        _state->process_push(*this);
    }

private:
    abstract_generic_index_i& _index;
    empty_state* _state;
};
}
