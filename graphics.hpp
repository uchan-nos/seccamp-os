#ifndef GRAPHICS_HPP_
#define GRAPHICS_HPP_

#include <stdint.h>
#include "bootparam.h"

extern unsigned char _binary_hankaku_bin_start[];
extern unsigned char _binary_hankaku_bin_end[];
extern unsigned char _binary_hankaku_bin_size[];

namespace bitnos::graphics
{
    struct Point
    {
        int x, y;
    };

    inline constexpr Point operator +(const Point& lhs, const Point& rhs)
    {
        return {lhs.x + rhs.x, lhs.y + rhs.y};
    }

    struct RectSize
    {
        unsigned int width, height;
    };

    inline constexpr Point operator +(const Point& p, const RectSize& s)
    {
        return {
            static_cast<decltype(p.x)>(p.x + s.width),
            static_cast<decltype(p.y)>(p.y + s.width)
        };
    }

    inline Point operator +(const RectSize& s, const Point& p)
    {
        return p + s;
    }

    struct Color
    {
        uint8_t r, g, b, a;
    };

    class PixelWriter
    {
        const uintptr_t frame_buffer_base_;
        const uint32_t pixels_per_scan_line_;
        const uint8_t bytes_per_pixel_;

    protected:
        uintptr_t CalcPixelAddr(const Point& pixel)
        {
            return frame_buffer_base_ +
                bytes_per_pixel_ * (pixels_per_scan_line_ * pixel.y + pixel.x);
        }

    public:
        PixelWriter(uintptr_t frame_buffer_base, uint32_t pixels_per_scan_line, uint32_t bytes_per_pixel)
            : frame_buffer_base_(frame_buffer_base),
              pixels_per_scan_line_(pixels_per_scan_line),
              bytes_per_pixel_(bytes_per_pixel)
        {}
        virtual ~PixelWriter() = default;
        PixelWriter(const PixelWriter&) = delete;
        PixelWriter& operator =(const PixelWriter&) = delete;
        PixelWriter(PixelWriter&&) = delete;
        PixelWriter& operator =(PixelWriter&&) = delete;

        virtual void Write(const Point& position, const Color& color) = 0;
        virtual void DrawRect(const Point& position, const RectSize& size, const Color& color);
    };

    class PixelWriterRedGreenBlueReserved8BitPerColor : public PixelWriter
    {
    public:
        PixelWriterRedGreenBlueReserved8BitPerColor(const GraphicMode *mode)
            : PixelWriter(mode->frame_buffer_base, mode->pixels_per_scan_line, 4)
        {}

        virtual void Write(const Point& position, const Color& color);
        virtual void DrawRect(const Point& position, const RectSize& size, const Color& color);
    };

    class PixelWriterBlueGreenRedReserved8BitPerColor : public PixelWriter
    {
    public:
        PixelWriterBlueGreenRedReserved8BitPerColor(const GraphicMode *mode)
            : PixelWriter(mode->frame_buffer_base, mode->pixels_per_scan_line, 4)
        {}

        virtual void Write(const Point& position, const Color& color);
        virtual void DrawRect(const Point& position, const RectSize& size, const Color& color);
    };

    /** @brief DrawAscii draws an ascii character to the given pixel writer.
     * _binary_hankaku_bin_start font is used.
     *
     * @param w  Pixel writer to draw a character
     * @param position  Pixel position
     * @param ch  Ascii code
     */
    void DrawAscii(PixelWriter& w, const Point& position, char ch);

    /** @brief DrawRect draws a rectangle to the given pixel writer.
     *
     * @param w  Pixel writer to draw a rectangle
     * @param position  Pixel position
     * @param size  Size of the rectangle (x and y must be positive value)
     * @param color  Draw color
     */
    void DrawRect(
        PixelWriter& w,
        const Point& position,
        const RectSize& size,
        const Color& color);

}

#endif // GRAPHICS_HPP_
