#ifndef QUEUE_HPP_
#define QUEUE_HPP_

#include <stddef.h>
#include "errorcode.hpp"

namespace bitnos
{
    template <typename T, size_t N>
    class ArrayQueue
    {
        T data_[N];
        size_t read_pos_, write_pos_, count_;
        /*
         * read_pos_ points an element to be read.
         * write_pos_ points a blank position.
         */
    public:
        ArrayQueue()
            : read_pos_(0), write_pos_(0)
        {}

        Error Push(const T& value)
        {
            if (count_ == N)
            {
                return errorcode::kFull;
            }

            data_[write_pos_] = value;
            ++count_;
            ++write_pos_;
            if (write_pos_ == N)
            {
                write_pos_ = 0;
            }
            return errorcode::kSuccess;
        }

        Error Pop()
        {
            if (count_ == 0)
            {
                return errorcode::kEmpty;
            }

            --count_;
            ++read_pos_;
            if (read_pos_ == N)
            {
                read_pos_ = 0;
            }
            return errorcode::kSuccess;
        }

        size_t Count() const
        {
            return count_;
        }

        const T& Front() const
        {
            return data_[read_pos_];
        }
    };
}

#endif // QUEUE_HPP_
