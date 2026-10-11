// This file is part of PadOS.
//
// Copyright (c) 2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <Math/Rect.h>
#include <Math/Point.h>
#include <GUI/Color.h>

namespace PBlitterUtils
{

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template<typename TReadPixel, typename TWritePixel, typename TNextLine>
void CopyBitmap(TReadPixel&& readPixel, TWritePixel&& writePixel, TNextLine&& nextLine, const PIRect& rect)
{
    for (int32_t y = rect.top; y < rect.bottom; ++y)
    {
        for (int32_t x = rect.left; x < rect.right; ++x) {
            writePixel(readPixel());
        }
        nextLine();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

// Find the first destination sample at or beyond a source boundary, once per draw.
// Endpoint tests avoid searching when the entire span is inside the source.
inline int32_t FindBilinearSpanBoundary(float origin, float scale, int32_t count, int32_t boundary)
{
    if (origin >= float(boundary)) {
        return 0;
    }
    if (origin + float(count - 1) * scale < float(boundary)) {
        return count;
    }
    int32_t first = 0;
    int32_t last = count;
    while (first < last)
    {
        const int32_t middle = first + (last - first) / 2;
        if (origin + float(middle) * scale < float(boundary)) {
            first = middle + 1;
        } else {
            last = middle;
        }
    }
    return first;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template<typename TReadPixel, typename TWritePixel>
void ScaleBitmapBilinear(
    TReadPixel&& readPixel,
    TWritePixel&& writePixel,
    const PIRect& srcOrigRect,
    const PIRect& dstOrigRect,
    const PRect& srcRect,
    const PIRect& dstRect,
    const PIRect& srcBounds)
{
    const float scaleX = float(srcOrigRect.Width() - 1) / float(dstOrigRect.Width());
    const float scaleY = float(srcOrigRect.Height() - 1) / float(dstOrigRect.Height());
    const int32_t width = dstRect.Width();
    const int32_t height = dstRect.Height();
    const int32_t firstX = FindBilinearSpanBoundary(srcRect.left, scaleX, width, srcBounds.left);
    const int32_t lastX = FindBilinearSpanBoundary(srcRect.left, scaleX, width, srcBounds.right - 1);
    const int32_t firstY = FindBilinearSpanBoundary(srcRect.top, scaleY, height, srcBounds.top);
    const int32_t lastY = FindBilinearSpanBoundary(srcRect.top, scaleY, height, srcBounds.bottom - 1);

    auto interpolate = [](float offsetX, float offsetY, float v00, float v10, float v01, float v11)
    {
        const float offsetXinv = 1.0f - offsetX;
        const float offsetYinv = 1.0f - offsetY;

        return offsetXinv * offsetYinv * v00 +
               offsetX    * offsetYinv * v10 +
               offsetXinv * offsetY    * v01 +
               offsetX    * offsetY    * v11;
    };
    auto samplePixel = [&readPixel, &interpolate](
        int32_t srcX,
        int32_t nextX,
        int32_t srcY,
        int32_t nextY,
        float offsetX,
        float offsetY) PALWAYS_INLINE
    {
        const PColor p00 = readPixel(srcX, srcY);
        const PColor p01 = readPixel(srcX, nextY);
        const PColor p10 = readPixel(nextX, srcY);
        const PColor p11 = readPixel(nextX, nextY);

        return PColor::FromRGB32AFloat(
            interpolate(offsetX, offsetY, p00.GetRedFloat(), p10.GetRedFloat(), p01.GetRedFloat(), p11.GetRedFloat()),
            interpolate(offsetX, offsetY, p00.GetGreenFloat(), p10.GetGreenFloat(), p01.GetGreenFloat(), p11.GetGreenFloat()),
            interpolate(offsetX, offsetY, p00.GetBlueFloat(), p10.GetBlueFloat(), p01.GetBlueFloat(), p11.GetBlueFloat()),
            interpolate(offsetX, offsetY, p00.GetAlphaFloat(), p10.GetAlphaFloat(), p01.GetAlphaFloat(), p11.GetAlphaFloat()));
    };
    auto renderRow = [&firstX, &samplePixel, &srcBounds, &writePixel, &dstRect, &lastX, &srcRect, &scaleX, &width](
        int32_t y,
        int32_t srcY,
        int32_t nextY,
        float offsetY)
    {
        if (firstX > 0)
        {
            const PColor pixel = samplePixel(srcBounds.left, srcBounds.left, srcY, nextY, 0.0f, offsetY);
            for (int32_t x = 0; x < firstX; ++x) {
                writePixel(dstRect.left + x, dstRect.top + y, pixel);
            }
        }
        for (int32_t x = firstX; x < lastX; ++x)
        {
            const float srcXf = srcRect.left + float(x) * scaleX;
            const int32_t srcX = int32_t(srcXf);
            const float offsetX = srcXf - float(srcX);
            const PColor pixel = samplePixel(srcX, srcX + 1, srcY, nextY, offsetX, offsetY);
            writePixel(dstRect.left + x, dstRect.top + y, pixel);
        }
        if (lastX < width)
        {
            const int32_t srcX = srcBounds.right - 1;
            const PColor pixel = samplePixel(srcX, srcX, srcY, nextY, 0.0f, offsetY);
            for (int32_t x = lastX; x < width; ++x) {
                writePixel(dstRect.left + x, dstRect.top + y, pixel);
            }
        }
    };

    // Constant edge runs also handle one-pixel source axes. The interior loop needs no bounds checks.
    for (int32_t y = 0; y < firstY; ++y) {
        renderRow(y, srcBounds.top, srcBounds.top, 0.0f);
    }
    for (int32_t y = firstY; y < lastY; ++y)
    {
        const float srcYf = srcRect.top + float(y) * scaleY;
        const int32_t srcY = int32_t(srcYf);
        renderRow(y, srcY, srcY + 1, srcYf - float(srcY));
    }
    for (int32_t y = lastY; y < height; ++y) {
        renderRow(y, srcBounds.bottom - 1, srcBounds.bottom - 1, 0.0f);
    }
}

} // namespace PBlitterUtils
