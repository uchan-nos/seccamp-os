#ifndef COMMAND_HPP_
#define COMMAND_HPP_

#include "errorcode.hpp"

namespace bitnos::command
{
    using FuncType = void (int argc, char* argv[]);

    struct Command
    {
        const char* name;
        FuncType* func_ptr;
    };

    extern Command table[5];
}

#endif // COMMAND_HPP_
