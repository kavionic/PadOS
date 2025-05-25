#pragma once

#include <Math/Rect.h>
#include <Math/Point.h>
#include <GUI/Color.h>

namespace os::BlitterUtils
{

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template<typename TReadPixel, typename TWritePixel, typename TNextLine>
void CopyBitmap(TReadPixel&& readPixel, TWritePixel&& writePixel, TNextLine&& nextLine, const IRect& rect)
{
    for (int32_t y = rect.top; y < rect.bottom; ++y)
    {
        for (int32_t x = rect.left; x < rect.right; ++x)
        {
            writePixel(readPixel());
        }
        nextLine();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template<typename TReadPixel, typename TWritePixel>
void ScaleBitmapBilinear(TReadPixel&& readPixel, TWritePixel&& writePixel, const IRect& srcOrigRect, const IRect& dstOrigRect, const Rect& srcRect, const IRect& dstRect)
{
    const float scaleX = float(srcOrigRect.Width()  - 1) / float(dstOrigRect.Width());
    const float scaleY = float(srcOrigRect.Height() - 1) / float(dstOrigRect.Height());

    for (int y = 0; y < dstRect.Height(); ++y)
    {
        const float   srcYf = srcRect.top + float(y) * scaleY;
        const int32_t srcY = int32_t(srcYf);
        const float   offsetY = srcYf - float(srcY);

        Color p00 = readPixel(int32_t(srcRect.left), srcY);
        Color p01 = readPixel(int32_t(srcRect.left), srcY + 1);

        for (int x = 0; x < dstRect.Width(); ++x)
        {
            const float   srcXf = srcRect.left + float(x) * scaleX;
            const int32_t srcX = int32_t(srcXf);
            const float   offsetX = srcXf - float(srcX);

            const Color p10 = readPixel(srcX + 1, srcY);
            const Color p11 = readPixel(srcX + 1, srcY + 1);

            auto interpolate = [](float offsetX, float offsetY, float v00, float v10, float v01, float v11)
            {
                const float offsetXinv = 1.0f - offsetX;
                const float offsetYinv = 1.0f - offsetY;

                return offsetXinv * offsetYinv * v00 +
                       offsetX    * offsetYinv * v10 +
                       offsetXinv * offsetY    * v01 +
                       offsetX    * offsetY    * v11;
            };

            // Interpolate each channel.
            const Color pixel32 = Color::FromRGB32AFloat(
                interpolate(offsetX, offsetY, p00.GetRedFloat(),   p10.GetRedFloat(),   p01.GetRedFloat(),   p11.GetRedFloat()),
                interpolate(offsetX, offsetY, p00.GetGreenFloat(), p10.GetGreenFloat(), p01.GetGreenFloat(), p11.GetGreenFloat()),
                interpolate(offsetX, offsetY, p00.GetBlueFloat(),  p10.GetBlueFloat(),  p01.GetBlueFloat(),  p11.GetBlueFloat()),
                interpolate(offsetX, offsetY, p00.GetAlphaFloat(), p10.GetAlphaFloat(), p01.GetAlphaFloat(), p11.GetAlphaFloat())
            );

            p00 = p10;
            p01 = p11;

            writePixel(dstRect.left + x, dstRect.top + y, pixel32);
        }
    }
}

} // namespace os::BlitterUtils
