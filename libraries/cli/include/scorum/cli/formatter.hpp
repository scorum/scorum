#pragma once

#include <string>
#include <iomanip>
#include <sstream>
#include <memory>
#include <vector>

namespace scorum {
namespace cli {

#define DEFAULT_SCREEN_W 100

enum class formatter_alignment
{
    left = 0,
    center,
    right,
};

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

    formatter_alignment set_alignment(formatter_alignment alignment)
    {
        auto old_alignment = _alignment;
        _alignment = alignment;
        return old_alignment;
    }

    formatter_alignment alignment() const
    {
        return _alignment;
    }

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
        print_internal(val);
        if (end_line)
            print_endl();
    }

    template <typename FN, typename T>
    void print_field(const FN& field_name, const T& field_val, bool end_line = true) const
    {
        formatter fn;
        fn.print_raw(field_name, false);
        formatter fv;
        fv.print_raw(field_val, false);
        print_field(fn.str(), fv.str(), end_line);
    }

    void print_field(const std::string& field_name, const std::string& field_val, bool end_line = true) const
    {
        print_internal(field_name, field_name.size());
        size_t out_w = screen_w - field_name.size();
        if (field_val.size() < out_w)
        {
            auto old_alignment = alignment();
            if (alignment() != formatter_alignment::center)
                _alignment = formatter_alignment::right;
            print_internal(field_val, out_w);
            _alignment = old_alignment;
        }
        else
        {
            print_endl();
            print_internal(field_val);
        }
        if (end_line)
            print_endl();
    }

    template <typename T> void print_line(const T& symbol, bool end_line = true) const
    {
        formatter fs;
        fs.print_raw(symbol, false);
        char s = line_symbol;
        std::string ss(fs.str());
        if (ss.size() > 0)
            s = ss[0];
        print_line(s, end_line);
    }

    void print_line(const char symbol, bool end_line = true) const
    {
        print_internal(std::string(screen_w, symbol));
        if (end_line)
            print_endl();
    }

    void print_line(bool end_line) const
    {
        print_line(wrap_symbol, end_line);
    }

    void print_line() const
    {
        print_line(wrap_symbol);
    }

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

                print_internal(val_str, out_w);

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

    template <typename T> void print_internal(const T& val, size_t out_w = (size_t)0) const
    {
        switch (_alignment)
        {
        case formatter_alignment::left:
        case formatter_alignment::center:
            _out << std::left;
            break;
        case formatter_alignment::right:
            _out << std::right;
            break;
        default:;
        }

        if (!out_w)
        {
            out_w = screen_w;
            if (_alignment == formatter_alignment::right)
                _out << std::setw(out_w);
        }
        else
            _out << std::setw(out_w);

        if (_alignment == formatter_alignment::center)
        {
            std::stringstream buff;
            buff << val;
            std::string center_val(buff.str());
            std::stringstream empty;
            buff.swap(empty);
            if (center_val.size() < out_w - 1)
            {
                out_w -= center_val.size();
                out_w /= 2;
                buff << std::string(out_w, _out.fill());
            }
            buff << val;
            _out << buff.str();
        }
        else
        {
            _out << val;
        }
    }

public:
    const size_t screen_w;
    const char* wrap_symbol = "> ";
    const char line_symbol = '-';

private:
    stringstream_type _own_out;
    std::stringstream& _out;
    mutable formatter_alignment _alignment = formatter_alignment::left;
    mutable stringstream_type _sequence_ctx;
    mutable cell_context _cell_ctx;
    mutable table_context _table_ctx;
};
}
}
