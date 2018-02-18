#ifndef ERRORCODE_HPP_
#define ERRORCODE_HPP_

namespace bitnos::errorcode
{
    using Type = int;

    const Type kSuccess = 0;
    const Type kIndexOutOfRange = 1;
    const Type kFull = 2;
    const Type kEmpty = 3;
    const Type kNotImplemented = 4;
    const Type kInvalidValue = 5;
    const Type kNoEnoughMemory = 6;
    const Type kNoMSICapability = 7;
}

namespace bitnos
{
    using Error = errorcode::Type;

    constexpr bool IsError(Error code)
    {
        return code != errorcode::kSuccess;
    }

    template <typename T>
    struct WithError
    {
        using value_type = T;

        T value;
        Error error;
    };
}

#endif // ERRORCODE_HPP_
