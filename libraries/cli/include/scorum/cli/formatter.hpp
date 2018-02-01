#pragma once

#include <string>
#include <iomanip>
#include <sstream>
#include <memory>
#include <vector>

namespace scorum {
namespace cli {

#define DEFAULT_SCREEN_W 100

class formatter
{
public:
    formatter(std::stringstream& out, const size_t scr_w = DEFAULT_SCREEN_W)
        : screen_w(scr_w)
        , _out(out)
        , _cell_ctx(*this)
    {
    }
    formatter(const size_t scr_w = DEFAULT_SCREEN_W)
        : screen_w(scr_w)
        , _own_out(new std::stringstream)
        , _out(*_own_out)
        , _cell_ctx(*this)
    {
    }

    void clear();

    std::string str() const;

    template <typename T, typename... Types> void print_sequence(const T& val, const Types&... vals) const
    {
        _start_print_sequence();
        _print_sequence(val);
        print_sequence(vals...);
    }

    void print_sequence() const
    {
        _end_print_sequence();
    }

    template <typename T, typename... Types> std::string print_sequence2str(const T& val, const Types&... vals) const
    {
        formatter p;
        p.print_sequence(val, vals...);
        return p.str();
    }

    void print_endl() const
    {
        _out << "\n";
    }

    template <typename T> void print_raw(const T& val, bool end_line = true) const
    {
        _out << std::left;
        _out << val;
        if (end_line)
            print_endl();
    }

    template <typename T> void print_field(const std::string& field_name, const T& field_val) const
    {
        print_raw(field_name, false);
        size_t out_w = screen_w - field_name.size();
        _out << std::right << std::setw(out_w) << field_val;
        print_endl();
    }

    void print_field(const std::string& field_name, const std::string& field_val) const;

    void print_line(const char symbol = '-', bool end_line = true) const;

    template <typename T> void print_cell(const T& val, size_t w, size_t cell_count) const
    {
        if (cell_count > 0)
        {
            if (!_cell_ctx.is_init())
                _cell_ctx.reset(cell_count);

            if (cell_count != _cell_ctx.count)
            {
                print_endl();
                _cell_ctx.reset(cell_count);
            }
            else if (_cell_ctx.current == _cell_ctx.count)
            {
                print_endl();
                _cell_ctx.reset(cell_count);
            }

            formatter p;
            p.print_raw(val, false);
            std::string val_str = p.str();
            size_t out_w = w;
            size_t offset = _cell_ctx.offset;
            if (offset > 0)
            {
                if (offset < w)
                {
                    out_w = w - offset;
                    offset = 0;
                }
                else
                {
                    out_w = 0;
                    offset -= w;
                }
            }

            if (_cell_ctx.splitted)
            {
                val_str = wrap_symbol + val_str;
                _cell_ctx.splitted = false;
            }

            if (val_str.size() >= out_w)
            {
                if (_cell_ctx.current + 1 < _cell_ctx.count)
                {
                    val_str += " ";
                }
                out_w = val_str.size();
                _cell_ctx.offset += out_w - w;
            }
            else
            {
                _cell_ctx.offset = offset;
            }

            if (_cell_ctx.left_w >= out_w)
            {
                _cell_ctx.left_w -= out_w;

                _out << std::left << std::setw(out_w);
                _out << val_str;

                _cell_ctx.current++;
            }
            else if (out_w < screen_w)
            {
                print_endl();
                _cell_ctx.left_w = screen_w;
                _cell_ctx.offset = 0;
                _cell_ctx.splitted = true;
                print_cell(val_str, w, cell_count);
            }
            else
            {
                if (_cell_ctx.left_w < screen_w)
                    print_endl();
                _cell_ctx.left_w = screen_w;
                _cell_ctx.offset = 0;
                val_str = wrap_symbol + val_str;
                print_raw(val_str);
                _cell_ctx.current++;
                _cell_ctx.splitted = true;
            }
        }
        else
        {
            print_raw(val);
        }
    }

    template <typename... Types> bool create_table(const size_t& w, const Types&... ws) const
    {
        if (_table_ctx.is_init())
        {
            _cell_ctx.reset();
            _table_ctx.reset();
        }
        _table_ctx.ws.push_back(w);
        bool ret = true;
        ret &= create_table(ws...);
        return ret;
    }

    bool create_table() const;

    template <typename T> bool print_cell(const T& val) const
    {
        if (_table_ctx.is_init())
        {
            if (!_table_ctx.opened)
            {
                _cell_ctx.reset();
                _table_ctx.opened = true;
            }
            if (_cell_ctx.current >= _table_ctx.ws.size())
            {
                print_endl();
                _cell_ctx.reset();
            }
            print_cell(val, _table_ctx.ws[_cell_ctx.current], _table_ctx.ws.size());
        }
        else
        {
            return false;
        }
        return true;
    }

private:
    void _start_print_sequence() const;

    template <typename T> void _print_sequence(const T& val) const
    {
        if (_sequence_ctx)
            (*_sequence_ctx) << val;
    }

    void _end_print_sequence() const;

    using stringstream_type = std::unique_ptr<std::stringstream>;

    struct cell_context
    {
        cell_context(const formatter& p)
            : screen_w(p.screen_w)
        {
        }

        const size_t screen_w;

        size_t current = 0;
        size_t count = 0;
        size_t left_w = 0;
        size_t offset = 0;
        bool splitted = false;

        bool is_init() const
        {
            return (current + count) > 0;
        }

        void reset(size_t cell_count = 0)
        {
            current = 0;
            count = cell_count;
            left_w = screen_w;
            offset = 0;
            splitted = false;
        }
    };

    struct table_context
    {
        std::vector<size_t> ws;
        bool saved = false;
        bool opened = false;

        bool is_init() const
        {
            return saved && !ws.empty();
        }

        void reset()
        {
            ws.clear();
            saved = false;
            opened = false;
        }
    };

public:
    const size_t screen_w;
    const char* wrap_symbol = "> ";

private:
    stringstream_type _own_out;
    std::stringstream& _out;
    mutable stringstream_type _sequence_ctx;
    mutable cell_context _cell_ctx;
    mutable table_context _table_ctx;
};
}
}
