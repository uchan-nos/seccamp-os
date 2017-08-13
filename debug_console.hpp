#ifndef DEBUG_CONSOLE_HPP_
#define DEBUG_CONSOLE_HPP_

#include "graphics.hpp"

namespace bitnos
{
    using graphics::PixelWriter;
    using graphics::Point;
    using graphics::RectSize;

    class DebugConsole
    {
        PixelWriter& writer_;
        RectSize size_;
        Point cur_;

        void Newline();
        void Backspace();
        void DrawCursor();
        void EraseCursor();

    public:
        DebugConsole(PixelWriter& writer, const RectSize& size);

        void PutChar(char ch);
        void PutStr(const char *s);
    };

    extern DebugConsole* default_debug_console;

    const char* const kDefaultPrompt = "$ ";

    //typedef void (DebugShellExecutorType)(DebugConsole& cons, const char* cmdline);
    class DebugShell
    {
        static const int kLineLength = 512;

        DebugConsole& cons_;
        int cur_;
        char buf_[kLineLength];

        const char* prompt_str_;

        void Exec();

    public:
        DebugShell(DebugConsole& cons);

        void PutChar(char ch);
    };
}

#endif // DEBUG_CONSOLE_HPP_
