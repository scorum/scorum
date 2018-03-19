#include <scorum/cli/formatter.hpp>

namespace scorum {
namespace cli {

void formatter::clear()
{
    _end_print_sequence();
    _cell_ctx.reset();
    _table_ctx.reset();
    std::stringstream empty;
    _out.swap(empty);
}

std::string formatter::str() const
{
    return _out.str();
}

bool formatter::create_table() const
{
    size_t table_w = 0;
    for (size_t w : _table_ctx.ws)
    {
        table_w += w;
    }
    if (table_w > screen_w)
    {
        _table_ctx.reset();
        return false;
    }
    _table_ctx.saved = true;
    return _table_ctx.is_init();
}

void formatter::_start_print_sequence() const
{
    if (!_sequence_ctx)
        _sequence_ctx.reset(new std::stringstream);
}

void formatter::_end_print_sequence() const
{
    if (_sequence_ctx)
    {
        _out << (*_sequence_ctx).str();
        _sequence_ctx.reset();
    }
}
}
}
