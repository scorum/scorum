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

void formatter::print_field(const std::string& field_name, const std::string& field_val) const
{
    print_raw(field_name, false);
    size_t out_w = screen_w - field_name.size();
    if (field_val.size() < out_w)
    {
        _out << std::right << std::setw(out_w) << field_val;
    }
    else
    {
        print_endl();
        _out << std::left << field_val;
    }
    print_endl();
}

void formatter::print_line(const char symbol /*= '-'*/, bool end_line /*= true*/) const
{
    _out << std::left << std::string(screen_w, symbol);
    if (end_line)
        print_endl();
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
