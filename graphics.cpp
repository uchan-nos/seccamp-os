/** @file * graphics.cpp provides basic graphic functions
 * such as pixel writers and ascii drawers.
 */

#include "graphics.hpp"

#include <stddef.h>

namespace bitnos::graphics
{

    void PixelWriter::DrawRect(const Point& position, const RectSize& size, const Color& color)
    {
        auto PutPixel = [&](const Point& diff)
        {
            this->Write(position + diff, color);
        };

        for (int dy = 0; dy < size.height; dy++)
        {
            for (int dx = 0; dx < size.width; dx++)
            {
                PutPixel({dx, dy});
            }
        }
    }

    void PixelWriterRedGreenBlueReserved8BitPerColor::Write(
        const Point& position, const Color& color)
    {
        auto p = reinterpret_cast<uint8_t*>(CalcPixelAddr(position));
        p[0] = color.r;
        p[1] = color.g;
        p[2] = color.b;
    }

    void PixelWriterRedGreenBlueReserved8BitPerColor::DrawRect(
        const Point& position, const RectSize& size, const Color& color)
    {
        auto PutPixel = [&](const Point& diff)
        {
            PixelWriterRedGreenBlueReserved8BitPerColor::Write(position + diff, color);
        };

        for (int dy = 0; dy < size.height; dy++)
        {
            for (int dx = 0; dx < size.width; dx++)
            {
                PutPixel({dx, dy});
            }
        }
    }

    void PixelWriterBlueGreenRedReserved8BitPerColor::Write(
        const Point& position, const Color& color)
    {
        auto p = reinterpret_cast<uint8_t*>(CalcPixelAddr(position));
        p[0] = color.b;
        p[1] = color.g;
        p[2] = color.r;
    }

    void PixelWriterBlueGreenRedReserved8BitPerColor::DrawRect(
        const Point& position, const RectSize& size, const Color& color)
    {
        auto PutPixel = [&](const Point& diff)
        {
            PixelWriterBlueGreenRedReserved8BitPerColor::Write(position + diff, color);
        };

        for (int dy = 0; dy < size.height; dy++)
        {
            for (int dx = 0; dx < size.width; dx++)
            {
                PutPixel({dx, dy});
            }
        }
    }

    void DrawAscii(PixelWriter& w, const Point& position, char ch)
    {
        unsigned char *p = _binary_hankaku_bin_start +
            16 * static_cast<size_t>(ch);
        auto PutPixel = [&](const Point& diff, const Color& color)
        {
            w.Write(position + diff, color);
        };
        const Color kBlack = {0, 0, 0, 0};

        for (int dy = 0; dy < 16; dy++)
        {
            auto pixels = p[dy];
            for (int dx = 0; dx < 8; dx++)
            {
                if (pixels & 0x80u)
                {
                    PutPixel({dx, dy}, kBlack);
                }
                pixels <<= 1;
            }
        }
    }

    void DrawRect(
        PixelWriter& w,
        const Point& position,
        const RectSize& size,
        const Color& color)
    {
        auto PutPixel = [&](const Point& diff)
        {
            w.Write(position + diff, color);
        };

        for (int dy = 0; dy < size.height; dy++)
        {
            for (int dx = 0; dx < size.width; dx++)
            {
                PutPixel({dx, dy});
            }
        }
    }

}
