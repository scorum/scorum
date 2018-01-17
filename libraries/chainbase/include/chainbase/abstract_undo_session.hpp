#pragma once

#include <memory>
#include <vector>
#include <boost/cstdint.hpp>

namespace chainbase {

struct abstract_undo_session
{
    virtual ~abstract_undo_session(){};

    virtual void push() = 0;
    virtual void squash() = 0;
    virtual void undo() = 0;
    virtual int64_t revision() const = 0;
};

using abstract_undo_session_ptr = std::unique_ptr<abstract_undo_session>;
using abstract_undo_session_list = std::vector<abstract_undo_session_ptr>;
}
