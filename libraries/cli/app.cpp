#include <scorum/cli/app.hpp>

#include <fc/io/stdio.hpp>
#include <fc/io/json.hpp>

#include <iostream>

#ifndef WIN32
#include <unistd.h>
#endif

#include <termios.h>

#ifdef HAVE_READLINE
#include <readline/readline.h>
#include <readline/history.h>
// I don't know exactly what version of readline we need.  I know the 4.2 version that ships on some macs is
// missing some functions we require.  We're developing against 6.3, but probably anything in the 6.x
// series is fine
#if RL_VERSION_MAJOR < 6
#ifdef _MSC_VER
#pragma message("You have an old version of readline installed that might not support some of the features we need")
#pragma message("Readline support will not be compiled in")
#else
#warning "You have an old version of readline installed that might not support some of the features we need"
#warning "Readline support will not be compiled in"
#endif
#undef HAVE_READLINE
#endif
#ifdef WIN32
#include <io.h>
#endif
#endif // HAVE_READLINE

namespace scorum {
namespace cli {

#ifdef HAVE_READLINE
namespace {

int getch_without_echo()
{
    int ch;
    struct termios t_old, t_new;

    tcgetattr(STDIN_FILENO, &t_old);
    t_new = t_old;
    t_new.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &t_new);

    ch = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &t_old);
    return ch;
}

char* _generator(const char*, int);
char** _completioner(const char*, int, int);

static struct bridge
{
    friend char* _generator(const char*, int);
    friend char** _completioner(const char*, int, int);

    void init(app* p)
    {
        _this = p;
        rl_attempted_completion_function = _completioner;
    }

private:
    char* dupstr(const char* s)
    {
        char* r;

        r = (char*)malloc((strlen(s) + 1));
        strcpy(r, s);
        return (r);
    }

    char* generator(const char* text, int state)
    {
        static size_t list_index, len;
        const char* name;

        if (!state)
        {
            list_index = 0;
            len = strlen(text);
        }

        auto& completions = _this->get_completions(_context_id);
        while (list_index < completions.size())
        {
            name = completions[list_index].c_str();
            list_index++;

            if (strncmp(name, text, len) == 0)
                return (dupstr(name));
        }

        return ((char*)nullptr);
    }

    char** completioner(const char* text, int start, int end)
    {
        char** matches;
        matches = (char**)nullptr;

        _context_id = _this->guess_completion_context(rl_line_buffer, text, start, end);
        if (_context_id != (int)completion_context_id::empty)
        {
            matches = rl_completion_matches((char*)text, &_generator);
        }
        else
            rl_bind_key('\t', rl_abort);

        return (matches);
    }

    static int _context_id;
    static app* _this;
} _bridge_for_readlinelib;

int bridge::_context_id = (int)completion_context_id::empty;
app* bridge::_this = nullptr;

char* _generator(const char* text, int state)
{
    return _bridge_for_readlinelib.generator(text, state);
}

char** _completioner(const char* text, int start, int end)
{
    return _bridge_for_readlinelib.completioner(text, start, end);
}
}
#endif // HAVE_READLINE

app::app()
{
}

app::~app()
{
    if (_run_complete.valid())
    {
        stop();
    }
}

void app::start()
{
    _run_complete = fc::async([&]() { run(); });
}
void app::stop()
{
    _run_complete.cancel();
    _run_complete.wait();
}
void app::wait()
{
    _run_complete.wait();
}

std::string app::get_secret(const std::string& prompt, bool show_asterisk)
{
    const char backspace_char = 127;
    const char return_char = '\n';

    std::string ret;
    unsigned char ch = 0;

    std::cout << prompt << " ";

    while ((ch = getch_without_echo()) != return_char)
    {
        if (ch == backspace_char)
        {
            if (ret.length() != 0)
            {
                if (show_asterisk)
                    std::cout << "\b \b";
                ret.resize(ret.length() - 1);
            }
        }
        else
        {
            ret += ch;
            if (show_asterisk)
                std::cout << '*';
        }
    }
    std::cout << std::endl;
    return ret;
}

void app::set_prompt(const std::string& prompt)
{
    _prompt = prompt;
}

void app::format_result(const std::string& command,
                        std::function<std::string(fc::variant, const fc::variants&)> formatter)
{
    _result_formatters[command] = formatter;
}

void app::getline(const fc::string& prompt, fc::string& line)
{
// getting file descriptor for C++ streams is near impossible
// so we just assume it's the same as the C stream...
#ifdef HAVE_READLINE
#ifndef WIN32
    if (isatty(fileno(stdin)))
#else
    // it's implied by
    // https://msdn.microsoft.com/en-us/library/f4s0ddew.aspx
    // that this is the proper way to do this on Windows, but I have
    // no access to a Windows compiler and thus,
    // no idea if this actually works
    if (_isatty(_fileno(stdin)))
#endif
    {
        _bridge_for_readlinelib.init(this);

        static fc::thread getline_thread("getline");
        getline_thread
            .async([&]() {
                char* line_read = nullptr;
                std::cout.flush(); // readline doesn't use cin, so we must manually
                // flush _out
                line_read = readline(prompt.c_str());
                if (line_read == nullptr)
                    FC_THROW_EXCEPTION(fc::eof_exception, "");
                rl_bind_key('\t', rl_complete);
                if (*line_read)
                    add_history(line_read);
                line = line_read;
                free(line_read);
            })
            .wait();
    }
    else
#endif
    {
        std::cout << prompt;
        fc::getline(fc::cin, line);
        return;
    }
}

void app::run()
{
    while (!_run_complete.canceled())
    {
        try
        {
            std::string line;
            try
            {
                getline(_prompt.c_str(), line);
            }
            catch (const fc::eof_exception&)
            {
                break;
            }
            if (line.empty())
                continue;
            std::cout << line << "\n";
            line += char(EOF);
            fc::variants args = fc::json::variants_from_string(line);
            if (args.size() == 0)
                continue;

            const std::string& command = args[0].get_string();

            auto result = process_command(command, fc::variants(args.begin() + 1, args.end()));
            auto itr = _result_formatters.find(command);
            if (itr == _result_formatters.end())
            {
                std::cout << fc::json::to_pretty_string(result) << "\n";
            }
            else
                std::cout << itr->second(result, args) << "\n";
        }
        catch (fc::canceled_exception&)
        {
        } // expected exception
        catch (const fc::exception& e)
        {
            std::cout << e.to_detail_string() << "\n";
        }
    }
}
}
}
