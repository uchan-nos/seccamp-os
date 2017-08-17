#include "debug_console.hpp"

#include <string.h>

#include "command.hpp"
#include "printk.hpp"

namespace bitnos
{
    using graphics::Point;
    using graphics::RectSize;

    DebugConsole* default_debug_console = nullptr;

    namespace
    {
        Point ToPixel(const Point& cur)
        {
            return {8 * cur.x, 16 * cur.y};
        }
    }

    DebugConsole::DebugConsole(PixelWriter& writer, const RectSize& size)
        : writer_(writer), size_(size), cur_({0, 0})
    {}

    void DebugConsole::Newline()
    {
        cur_.x = 0;
        if (cur_.y < size_.height - 1)
        {
            ++cur_.y;
        }
        else
        {
            // scroll
        }
    }

    void DebugConsole::Backspace()
    {
        if (cur_.x > 0)
        {
            --cur_.x;
            return;
        }

        cur_.x = static_cast<decltype(cur_.x)>(size_.width - 1);
        if (cur_.y > 0)
        {
            --cur_.y;
        }
        else
        {
            // scroll
        }
    }

    void DebugConsole::DrawCursor()
    {
        DrawRect(writer_, ToPixel(cur_), {8, 16}, {255, 255, 255, 0});
        DrawAscii(writer_, ToPixel(cur_), '_');
    }

    void DebugConsole::EraseCursor()
    {
        DrawRect(writer_, ToPixel(cur_), {8, 16}, {255, 255, 255, 0});
    }

    void DebugConsole::PutChar(char ch)
    {
        EraseCursor();

        switch (ch)
        {
        case '\n':
            Newline();
            return;
        case '\b':
            Backspace();
            DrawCursor();
            return;
        }

        DrawAscii(writer_, ToPixel(cur_), ch);
        if (cur_.x < size_.width - 1)
        {
            ++cur_.x;
        }
        else
        {
            Newline();
        }

        DrawCursor();
    }

    void DebugConsole::PutStr(const char *s)
    {
        while (*s)
        {
            PutChar(*s++);
        }
    }

    void DebugShell::Exec()
    {
        const auto len = strlen(buf_);
        if (len == 0)
        {
            return;
        }

        char s[len + 1];

        int argc = 0;
        char *argv[128];

        int i = 0;
        while (true)
        {
            for (; i < len && buf_[i] == ' '; ++i)
            {
                s[i] = '\0';
            }
            if (i > len)
            {
                break;
            }
            argv[argc++] = s + i;
            for (; i < len && buf_[i] != ' '; ++i)
            {
                s[i] = buf_[i];
            }
            s[i++] = '\0';
        }

        bool found = false;
        for (const auto& cmd : command::table)
        {
            if (strcmp(argv[0], cmd.name) == 0)
            {
                found = true;
                cmd.func_ptr(argc, argv);
                break;
            }
        }

        if (!found)
        {
            printk("no such command: %s\n", argv[0]);
        }
    }

    DebugShell::DebugShell(DebugConsole& cons)
        : cons_(cons), cur_(0), prompt_str_(kDefaultPrompt)
    {
        cons_.PutStr(prompt_str_);
    }

    void DebugShell::PutChar(char ch)
    {
        if (cur_ > kLineLength-1)
        {
            return;
        }

        switch (ch)
        {
        case '\n':
            buf_[cur_] = '\0';
            cur_ = 0;
            cons_.PutChar('\n');
            Exec();
            cons_.PutStr(prompt_str_);
            return;
        case '\b':
            if (cur_ == 0)
            {
                return;
            }
            --cur_;
            break;
        default:
            buf_[cur_++] = ch;
        }

        cons_.PutChar(ch);
    }
}
