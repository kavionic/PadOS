// This file is part of PadOS.
//
// Copyright (C) 1999-2026 Kurt Skauen <http://kavionic.com/>
//
// PadOS is free software : you can redistribute it and / or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// PadOS is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with PadOS. If not, see <http://www.gnu.org/licenses/>.
///////////////////////////////////////////////////////////////////////////////

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <algorithm>
#include <array>
#include <cmath>

#include "ApplicationServer/DisplayDriver.h"
#include <ApplicationServer/ServerBitmap.h>
#include <ApplicationServer/ApplicationServer.h>

#include <GUI/Bitmap.h>
#include <GUI/Color.h>
#include <Utils/Utils.h>
#include <Utils/UTF8Utils.h>

#include "ApplicationServer/Fonts/SansSerif_14.h"
#include "ApplicationServer/Fonts/SansSerif_20.h"
#include "ApplicationServer/Fonts/SansSerif_72.h"


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PDisplayDriver::PDisplayDriver() : m_CursorHotSpot(0, 0)
{
//    m_pcMouseImage = nullptr;
//    m_pcMouseSprite = nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PDisplayDriver::~PDisplayDriver()
{
//    delete m_pcMouseSprite;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDisplayDriver::Close(void)
{
}

///////////////////////////////////////////////////////////////////////////////
/// Get offset between raster-area start and frame-buffer start
///
/// \par Description:
///     Should return the offset from the frame-buffer area's logical
///     address to the actual start of the frame-buffer. This should
///     normally be 0 but might be between 0-4095 if the frame-buffer
///     is not page-aligned (Like when running under WMVare).
/// \par Note:
///     The driver should try to avoid using this offset since it will give
///     user-space applications access to the memory area between the start
///     of the raster-area and the start of the bitmap.
/// \return
/// \sa
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int PDisplayDriver::GetFramebufferOffset()
{
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

//void DisplayDriver::SetCursorBitmap(mouse_ptr_mode eMode, const IPoint& cHotSpot, const void* pRaster, int nWidth, int nHeight)
//{
//    SrvSprite::Hide();
//
//    m_cCursorHotSpot = cHotSpot;
//    delete m_pcMouseSprite;
//
//    if (m_pcMouseImage != NULL) {
//        m_pcMouseImage->Release();
//    }
//    m_pcMouseImage = new SrvBitmap(nWidth, nHeight, ColorSpace::CMAP8);
//
//    uint8_t* dstRaster = m_pcMouseImage->m_Raster;
//    const uint8_t* srcRaster = static_cast<const uint8_t*>(pRaster);
//
//    for (int i = 0; i < nWidth * nHeight; ++i) {
//        uint8_t anPalette[] = { 255, 0, 0, 63 };
//        if (srcRaster[i] < 4) {
//            dstRaster[i] = anPalette[srcRaster[i]];
//        } else {
//            dstRaster[i] = 255;
//        }
//    }
//    if (m_pcMouseImage != NULL) {
//        m_pcMouseSprite = new SrvSprite(IRect(0, 0, nWidth, nHeight), m_cMousePos, m_cCursorHotSpot, g_pcScreenBitmap, m_pcMouseImage);
//    }
//    SrvSprite::Unhide();
//}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDisplayDriver::MouseOn()
{
//    if (m_pcMouseSprite == NULL) {
//        m_pcMouseSprite = new SrvSprite(IRect(0, 0, m_pcMouseImage->m_nWidth, m_pcMouseImage->m_nHeight), m_cMousePos, m_cCursorHotSpot, g_pcScreenBitmap, m_pcMouseImage);
//    } else {
//        p_system_log<PLogSeverity::WARNING>(LogCategoryAppServer, "DisplayDriver::MouseOn() called while mouse visible.");
//    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDisplayDriver::MouseOff()
{
//    if (m_pcMouseSprite == NULL) {
//        p_system_log<PLogSeverity::WARNING>(LogCategoryAppServer, "VDisplayDriver::MouseOff() called while mouse hidden");
//    }
//    delete m_pcMouseSprite;
//    m_pcMouseSprite = NULL;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDisplayDriver::SetMousePos(PIPoint cNewPos)
{
//    if (m_pcMouseSprite != NULL) {
//        m_pcMouseSprite->MoveTo(cNewPos);
//    }
    m_MousePos = cNewPos;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PColor PDisplayDriver::GetPaletteEntry(uint8_t index)
{
    return PColor::FromCMAP8(index);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDisplayDriver::FillBlit8(uint8_t* dst, int nMod, int W, int H, uint8_t nColor)
{
    int X, Y;

    for (Y = 0; Y < H; Y++)
    {
        for (X = 0; X < W; X++) {
            *dst++ = nColor;
        }
        dst += nMod;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDisplayDriver::FillBlit16(uint16_t* dst, int nMod, int W, int H, uint16_t nColor)
{
    int X, Y;

    for (Y = 0; Y < H; Y++)
    {
        for (X = 0; X < W; X++) {
            *dst++ = nColor;
        }
        dst += nMod;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDisplayDriver::FillBlit24(uint8_t* dst, int nMod, int W, int H, uint32_t nColor)
{
    int X, Y;

    for (Y = 0; Y < H; Y++)
    {
        for (X = 0; X < W; X++)
        {
            *dst++ = nColor & 0xff;
            *((uint16_t*)dst) = uint16_t(nColor >> 8);
            dst += 2;
        }
        dst += nMod;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDisplayDriver::FillBlit32(uint32_t* dst, int nMod, int W, int H, uint32_t nColor)
{
    int X, Y;

    for (Y = 0; Y < H; Y++)
    {
        for (X = 0; X < W; X++) {
            *dst++ = nColor;
        }
        dst += nMod;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDisplayDriver::WritePixel(PSrvBitmap* bitmap, const PIPoint& pos, PColor color)
{
    switch (bitmap->m_ColorSpace)
    {
        case PEColorSpace::RGB15:
            *reinterpret_cast<uint16_t*>(&bitmap->m_Raster[pos.x * 2 + pos.y * bitmap->m_BytesPerLine]) = color.GetColor15();
            break;
        case PEColorSpace::RGB16:
            *reinterpret_cast<uint16_t*>(&bitmap->m_Raster[pos.x * 2 + pos.y * bitmap->m_BytesPerLine]) = color.GetColor16();
            break;
        case PEColorSpace::RGB32:
            *reinterpret_cast<uint32_t*>(&bitmap->m_Raster[pos.x * 4 + pos.y * bitmap->m_BytesPerLine]) = color.GetColor32();
            break;
        default:
            p_system_log<PLogSeverity::ERROR>(LogCategoryAppServer, "DisplayDriver::WritePixel() unknown color space {}.", int(bitmap->m_ColorSpace));
            break;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static bool EdgeCrossesScanline(float start, float end, float sample)
{
    return (start <= sample && end > sample)
        || (end <= sample && start > sample);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static int FirstCoveredPixel(float boundary)
{
    return int(std::ceil(boundary - 0.5f));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static int LastCoveredPixel(float boundary)
{
    return int(std::ceil(boundary - 0.5f)) - 1;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

struct TriangleSpanInterval
{
    float Start;
    float End;
};

struct PolygonScanEdge
{
    int   StartScan = 0;
    int   LastScan = 0;
    float Intersection = 0.0f;
    float Step = 0.0f;
};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static void AddPolygonScanEdge(std::vector<PolygonScanEdge>& scanEdges,
                               const PPoint& edgeStart,
                               const PPoint& edgeEnd,
                               int firstScan,
                               int lastScan,
                               bool useHorizontalSpans)
{
    const float edgeStartScan = useHorizontalSpans ? edgeStart.y : edgeStart.x;
    const float edgeEndScan = useHorizontalSpans ? edgeEnd.y : edgeEnd.x;
    const float scanDelta = edgeEndScan - edgeStartScan;
    if (std::abs(scanDelta) < 1.0e-6f) {
        return;
    }

    const float edgeStartSpan = useHorizontalSpans ? edgeStart.x : edgeStart.y;
    const float edgeEndSpan = useHorizontalSpans ? edgeEnd.x : edgeEnd.y;
    const float spanDelta = edgeEndSpan - edgeStartSpan;
    const int edgeFirstScan = std::max(firstScan, FirstCoveredPixel(std::min(edgeStartScan, edgeEndScan)));
    const int edgeLastScan = std::min(lastScan, LastCoveredPixel(std::max(edgeStartScan, edgeEndScan)));
    if (edgeFirstScan > edgeLastScan) {
        return;
    }

    const float firstSample = float(edgeFirstScan) + 0.5f;
    const float firstRatio = (firstSample - edgeStartScan) / scanDelta;
    scanEdges.push_back({ edgeFirstScan, edgeLastScan, edgeStartSpan + spanDelta * firstRatio, spanDelta / scanDelta });
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static bool GetTriangleSpanInterval(const std::array<PPoint, 3>& triangle,
                                    float samplePosition,
                                    bool useHorizontalSpans,
                                    TriangleSpanInterval& outInterval)
{
    const PPoint delta1 = triangle[1] - triangle[0];
    const PPoint delta2 = triangle[2] - triangle[0];
    const float area = delta1.x * delta2.y - delta1.y * delta2.x;
    if (std::abs(area) < 1e-6f) {
        return false;
    }

    std::array<float, 3> intersections;
    size_t intersectionCount = 0;
    for (size_t edgeIndex = 0; edgeIndex < triangle.size(); ++edgeIndex)
    {
        const PPoint& edgeStart = triangle[edgeIndex];
        const PPoint& edgeEnd = triangle[(edgeIndex + 1) % triangle.size()];
        const float edgeStartScan = useHorizontalSpans ? edgeStart.y : edgeStart.x;
        const float edgeEndScan = useHorizontalSpans ? edgeEnd.y : edgeEnd.x;
        if (EdgeCrossesScanline(edgeStartScan, edgeEndScan, samplePosition))
        {
            const float ratio = (samplePosition - edgeStartScan) / (edgeEndScan - edgeStartScan);
            intersections[intersectionCount++] = useHorizontalSpans
                ? edgeStart.x + (edgeEnd.x - edgeStart.x) * ratio
                : edgeStart.y + (edgeEnd.y - edgeStart.y) * ratio;
        }
    }

    if (intersectionCount != 2) {
        return false;
    }

    outInterval.Start = std::min(intersections[0], intersections[1]);
    outInterval.End = std::max(intersections[0], intersections[1]);
    return outInterval.Start < outInterval.End;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDisplayDriver::FillTriangleUnion(PSrvBitmap* bitmap, const PIRect& clipRect,
                                       std::span<const std::array<PPoint, 3>> triangles,
                                       PDrawingMode mode)
{
    if (triangles.empty() || clipRect.Width() <= 0 || clipRect.Height() <= 0) {
        return;
    }

    bool hasBounds = false;
    float minX = 0.0f;
    float minY = 0.0f;
    float maxX = 0.0f;
    float maxY = 0.0f;
    for (const std::array<PPoint, 3>& triangle : triangles)
    {
        const PPoint delta1 = triangle[1] - triangle[0];
        const PPoint delta2 = triangle[2] - triangle[0];
        const float area = delta1.x * delta2.y - delta1.y * delta2.x;
        if (std::abs(area) < 1e-6f) {
            continue;
        }

        for (const PPoint& point : triangle)
        {
            if (!hasBounds)
            {
                minX = point.x;
                minY = point.y;
                maxX = point.x;
                maxY = point.y;
                hasBounds = true;
            }
            else
            {
                minX = std::min(minX, point.x);
                minY = std::min(minY, point.y);
                maxX = std::max(maxX, point.x);
                maxY = std::max(maxY, point.y);
            }
        }
    }

    if (!hasBounds || minX >= maxX || minY >= maxY) {
        return;
    }

    const bool useHorizontalSpans = (maxX - minX) >= (maxY - minY);
    const float mergeTolerance = 1e-5f;
    std::vector<TriangleSpanInterval> intervals;
    intervals.reserve(triangles.size());

    if (useHorizontalSpans)
    {
        const int firstScanY = std::max(clipRect.top, FirstCoveredPixel(minY));
        const int lastScanY = std::min(clipRect.bottom - 1, LastCoveredPixel(maxY));
        for (int scanY = firstScanY; scanY <= lastScanY; ++scanY)
        {
            const float sampleY = float(scanY) + 0.5f;
            intervals.clear();
            for (const std::array<PPoint, 3>& triangle : triangles)
            {
                TriangleSpanInterval interval;
                if (GetTriangleSpanInterval(triangle, sampleY, true, interval)) {
                    intervals.push_back(interval);
                }
            }
            std::sort(intervals.begin(), intervals.end(),
                [](const TriangleSpanInterval& lhs, const TriangleSpanInterval& rhs)
                {
                    return (lhs.Start < rhs.Start)
                        || (lhs.Start == rhs.Start && lhs.End < rhs.End);
                });

            size_t intervalIndex = 0;
            while (intervalIndex < intervals.size())
            {
                float spanStartFloat = intervals[intervalIndex].Start;
                float spanEndFloat = intervals[intervalIndex].End;
                ++intervalIndex;
                while (intervalIndex < intervals.size()
                       && intervals[intervalIndex].Start <= spanEndFloat + mergeTolerance)
                {
                    spanEndFloat = std::max(spanEndFloat, intervals[intervalIndex].End);
                    ++intervalIndex;
                }
                const int spanStart = std::max(clipRect.left, FirstCoveredPixel(spanStartFloat));
                const int spanEnd = std::min(clipRect.right - 1, LastCoveredPixel(spanEndFloat));
                if (spanStart <= spanEnd) {
                    FillRect(bitmap, PIRect(spanStart, scanY, spanEnd + 1, scanY + 1));
                }
            }
        }
    }
    else
    {
        const int firstScanX = std::max(clipRect.left, FirstCoveredPixel(minX));
        const int lastScanX = std::min(clipRect.right - 1, LastCoveredPixel(maxX));
        for (int scanX = firstScanX; scanX <= lastScanX; ++scanX)
        {
            const float sampleX = float(scanX) + 0.5f;
            intervals.clear();
            for (const std::array<PPoint, 3>& triangle : triangles)
            {
                TriangleSpanInterval interval;
                if (GetTriangleSpanInterval(triangle, sampleX, false, interval)) {
                    intervals.push_back(interval);
                }
            }
            std::sort(intervals.begin(), intervals.end(),
                [](const TriangleSpanInterval& lhs, const TriangleSpanInterval& rhs)
                {
                    return (lhs.Start < rhs.Start)
                        || (lhs.Start == rhs.Start && lhs.End < rhs.End);
                });

            size_t intervalIndex = 0;
            while (intervalIndex < intervals.size())
            {
                float spanStartFloat = intervals[intervalIndex].Start;
                float spanEndFloat = intervals[intervalIndex].End;
                ++intervalIndex;
                while (intervalIndex < intervals.size()
                       && intervals[intervalIndex].Start <= spanEndFloat + mergeTolerance)
                {
                    spanEndFloat = std::max(spanEndFloat, intervals[intervalIndex].End);
                    ++intervalIndex;
                }
                const int spanStart = std::max(clipRect.top, FirstCoveredPixel(spanStartFloat));
                const int spanEnd = std::min(clipRect.bottom - 1, LastCoveredPixel(spanEndFloat));
                if (spanStart <= spanEnd) {
                    FillRect(bitmap, PIRect(scanX, spanStart, scanX + 1, spanEnd + 1));
                }
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDisplayDriver::FillPolygon(PSrvBitmap* bitmap, std::span<const PIRect> clipRects, std::span<const PPoint> points, PDrawingMode mode)
{
    if (points.size() < 3 || clipRects.empty()) {
        return;
    }

    float minPointX = points[0].x;
    float minPointY = points[0].y;
    float maxPointX = points[0].x;
    float maxPointY = points[0].y;
    for (size_t pointIndex = 1; pointIndex < points.size(); ++pointIndex)
    {
        minPointX = std::min(minPointX, points[pointIndex].x);
        minPointY = std::min(minPointY, points[pointIndex].y);
        maxPointX = std::max(maxPointX, points[pointIndex].x);
        maxPointY = std::max(maxPointY, points[pointIndex].y);
    }

    if (minPointX >= maxPointX || minPointY >= maxPointY) {
        return;
    }

    const bool useHorizontalSpans = (maxPointX - minPointX) >= (maxPointY - minPointY);
    const float minScan = useHorizontalSpans ? minPointY : minPointX;
    const float maxScan = useHorizontalSpans ? maxPointY : maxPointX;
    const float minSpan = useHorizontalSpans ? minPointX : minPointY;
    const float maxSpan = useHorizontalSpans ? maxPointX : maxPointY;
    const int firstPolygonScan = FirstCoveredPixel(minScan);
    const int lastPolygonScan = LastCoveredPixel(maxScan);
    const int firstPolygonSpan = FirstCoveredPixel(minSpan);
    const int lastPolygonSpan = LastCoveredPixel(maxSpan);

    bool hasValidClip   = false;
    int firstClipScan   = 0;
    int lastClipScan    = 0;
    int firstClipSpan   = 0;
    int lastClipSpan    = 0;

    for (const PIRect& clipRect : clipRects)
    {
        if (clipRect.Width() <= 0 || clipRect.Height() <= 0) {
            continue;
        }

        const int clipScanStart = useHorizontalSpans ? clipRect.top         : clipRect.left;
        const int clipScanEnd   = useHorizontalSpans ? clipRect.bottom - 1  : clipRect.right - 1;
        const int clipSpanStart = useHorizontalSpans ? clipRect.left        : clipRect.top;
        const int clipSpanEnd   = useHorizontalSpans ? clipRect.right - 1   : clipRect.bottom - 1;

        if (!hasValidClip)
        {
            firstClipScan   = clipScanStart;
            lastClipScan    = clipScanEnd;
            firstClipSpan   = clipSpanStart;
            lastClipSpan    = clipSpanEnd;
            hasValidClip    = true;
        }
        else
        {
            firstClipScan   = std::min(firstClipScan, clipScanStart);
            lastClipScan    = std::max(lastClipScan, clipScanEnd);
            firstClipSpan   = std::min(firstClipSpan, clipSpanStart);
            lastClipSpan    = std::max(lastClipSpan, clipSpanEnd);
        }
    }

    if (!hasValidClip || firstClipSpan > lastPolygonSpan || lastClipSpan < firstPolygonSpan) {
        return;
    }

    const int firstScan = std::max(firstPolygonScan, firstClipScan);
    const int lastScan  = std::min(lastPolygonScan, lastClipScan);
    if (firstScan > lastScan) {
        return;
    }

    std::vector<PolygonScanEdge> scanEdges;
    scanEdges.reserve(points.size());

    for (size_t edgeIndex = 0; edgeIndex < points.size(); ++edgeIndex) {
        AddPolygonScanEdge(scanEdges, points[edgeIndex], points[(edgeIndex + 1) % points.size()], firstScan, lastScan, useHorizontalSpans);
    }

    if (scanEdges.empty()) {
        return;
    }

    std::sort(scanEdges.begin(), scanEdges.end(),
        [](const PolygonScanEdge& lhs, const PolygonScanEdge& rhs)
        {
            return (lhs.StartScan < rhs.StartScan)
                || (lhs.StartScan == rhs.StartScan && lhs.Intersection < rhs.Intersection);
        });

    std::vector<PolygonScanEdge> activeEdges;
    std::vector<float> intersections;

    activeEdges.reserve(scanEdges.size());
    intersections.reserve(scanEdges.size());

    for (const PIRect& clipRect : clipRects)
    {
        if (clipRect.Width() <= 0 || clipRect.Height() <= 0) {
            continue;
        }

        const int scanClipStart = useHorizontalSpans ? clipRect.top : clipRect.left;
        const int scanClipEnd   = useHorizontalSpans ? clipRect.bottom - 1 : clipRect.right - 1;
        const int spanClipStart = useHorizontalSpans ? clipRect.left : clipRect.top;
        const int spanClipEnd   = useHorizontalSpans ? clipRect.right - 1 : clipRect.bottom - 1;

        const int firstClipScanClamped  = std::max(firstScan, scanClipStart);
        const int lastClipScanClamped   = std::min(lastScan, scanClipEnd);

        if (firstClipScanClamped > lastClipScanClamped || spanClipStart > lastPolygonSpan || spanClipEnd < firstPolygonSpan) {
            continue;
        }

        activeEdges.clear();
        size_t nextEdgeIndex = 0;
        while (nextEdgeIndex < scanEdges.size() && scanEdges[nextEdgeIndex].StartScan <= firstClipScanClamped)
        {
            const PolygonScanEdge& edge = scanEdges[nextEdgeIndex];
            if (edge.LastScan >= firstClipScanClamped)
            {
                PolygonScanEdge activeEdge = edge;
                activeEdge.Intersection += activeEdge.Step * float(firstClipScanClamped - activeEdge.StartScan);
                activeEdges.push_back(activeEdge);
            }
            ++nextEdgeIndex;
        }

        for (int scan = firstClipScanClamped; scan <= lastClipScanClamped; ++scan)
        {
            activeEdges.erase(std::remove_if(activeEdges.begin(), activeEdges.end(),
                [scan](const PolygonScanEdge& edge)
                {
                    return edge.LastScan < scan;
                }), activeEdges.end());

            while (nextEdgeIndex < scanEdges.size() && scanEdges[nextEdgeIndex].StartScan <= scan)
            {
                activeEdges.push_back(scanEdges[nextEdgeIndex]);
                ++nextEdgeIndex;
            }

            intersections.clear();
            for (const PolygonScanEdge& edge : activeEdges) {
                intersections.push_back(edge.Intersection);
            }
            std::sort(intersections.begin(), intersections.end());

            for (size_t intersectionIndex = 0; intersectionIndex + 1 < intersections.size(); intersectionIndex += 2)
            {
                const int spanStart = std::max(spanClipStart, FirstCoveredPixel(intersections[intersectionIndex]));
                const int spanEnd   = std::min(spanClipEnd, LastCoveredPixel(intersections[intersectionIndex + 1]));
                if (spanStart <= spanEnd)
                {
                    if (useHorizontalSpans) {
                        FillRect(bitmap, PIRect(spanStart, scan, spanEnd + 1, scan + 1));
                    } else {
                        FillRect(bitmap, PIRect(scan, spanStart, scan + 1, spanEnd + 1));
                    }
                }
            }

            for (PolygonScanEdge& edge : activeEdges) {
                edge.Intersection += edge.Step;
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDisplayDriver::FillTriangle(PSrvBitmap* bitmap, const PIRect& clipRect, const PIPoint& pos1, const PIPoint& pos2, const PIPoint& pos3, PDrawingMode mode)
{
    const std::array<PPoint, 3> points = {
        PPoint(pos1), PPoint(pos2), PPoint(pos3)
    };
    const std::array<PIRect, 1> clipRects = { clipRect };
    FillPolygon(bitmap, clipRects, points, mode);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDisplayDriver::FillTriangleFan(PSrvBitmap* bitmap, const PIRect& clipRect, std::span<const PPoint> points, PDrawingMode mode)
{
    if (points.size() < 3) {
        return;
    }

    std::vector<std::array<PPoint, 3>> triangles;
    triangles.reserve(points.size() - 2);
    for (size_t pointIndex = 1; pointIndex + 1 < points.size(); ++pointIndex) {
        triangles.push_back({ points[0], points[pointIndex], points[pointIndex + 1] });
    }
    FillTriangleUnion(bitmap, clipRect, triangles, mode);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDisplayDriver::FillTriangleStrip(PSrvBitmap* bitmap, const PIRect& clipRect, std::span<const PPoint> points, PDrawingMode mode)
{
    if (points.size() < 3) {
        return;
    }

    std::vector<std::array<PPoint, 3>> triangles;
    triangles.reserve(points.size() - 2);
    for (size_t pointIndex = 0; pointIndex + 2 < points.size(); ++pointIndex) {
        triangles.push_back({ points[pointIndex], points[pointIndex + 1], points[pointIndex + 2] });
    }
    FillTriangleUnion(bitmap, clipRect, triangles, mode);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDisplayDriver::FillRect(PSrvBitmap* bitmap, const PIRect& rect)
{
    int BltX, BltY, BltW, BltH;

    BltX = rect.left;
    BltY = rect.top;
    BltW = rect.Width();
    BltH = rect.Height();

    switch (bitmap->m_ColorSpace)
    {
        //    case ColorSpace::CMAP8:
        //      FillBlit8( bitmap->m_Raster + ((BltY * bitmap->m_BytesPerLine) + BltX),
        //       bitmap->m_BytesPerLine - BltW, BltW, BltH, nColor );
        //      break;
        case PEColorSpace::RGB15:
            FillBlit16((uint16_t*)&bitmap->m_Raster[BltY * bitmap->m_BytesPerLine + BltX * 2], bitmap->m_BytesPerLine / 2 - BltW, BltW, BltH, m_FgColor.GetColor15());
            break;
        case PEColorSpace::RGB16:
            FillBlit16((uint16_t*)&bitmap->m_Raster[BltY * bitmap->m_BytesPerLine + BltX * 2], bitmap->m_BytesPerLine / 2 - BltW, BltW, BltH, m_FgColor.GetColor16());
            break;
        case PEColorSpace::RGB24:
            FillBlit24( &bitmap->m_Raster[ BltY * bitmap->m_BytesPerLine + BltX * 3 ], bitmap->m_BytesPerLine - BltW * 3, BltW, BltH, m_FgColor.GetColor32() );
            break;
        case PEColorSpace::RGB32:
            FillBlit32((uint32_t*)&bitmap->m_Raster[BltY * bitmap->m_BytesPerLine + BltX * 4], bitmap->m_BytesPerLine / 4 - BltW, BltW, BltH, m_FgColor.GetColor32());
            break;
        default:
            p_system_log<PLogSeverity::ERROR>(LogCategoryAppServer, "DisplayDriver::FillRect() unknown color space {}.", int(bitmap->m_ColorSpace));
            break;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static inline void Blit(uint8_t* Src, uint8_t* Dst, int SMod, int DMod, int W, int H, bool Rev)
{
    int   i;
    int       X, Y;
    uint32_t* LSrc;
    uint32_t* LDst;

    if (Rev)
    {
        for (Y = 0; Y < H; Y++)
        {
            for (X = 0; (X < W) && ((uint32_t(Src - 3)) & 3); X++) {
                *Dst-- = *Src--;
            }

            LSrc = (uint32_t*)(((uint32_t)Src) - 3);
            LDst = (uint32_t*)(((uint32_t)Dst) - 3);

            i = (W - X) / 4;

            X += i * 4;

            for (; i; i--) {
                *LDst-- = *LSrc--;
            }

            Src = (uint8_t*)(((uint32_t)LSrc) + 3);
            Dst = (uint8_t*)(((uint32_t)LDst) + 3);

            for (; X < W; X++) {
                *Dst-- = *Src--;
            }

            Dst -= (int32_t)DMod;
            Src -= (int32_t)SMod;
        }
    }
    else
    {
        for (Y = 0; Y < H; Y++)
        {
            for (X = 0; (X < W) && (((uint32_t)Src) & 3); ++X) {
                *Dst++ = *Src++;
            }

            LSrc = (uint32_t*)Src;
            LDst = (uint32_t*)Dst;

            i = (W - X) / 4;

            X += i * 4;

            for (; i; i--) {
                *LDst++ = *LSrc++;
            }

            Src = (uint8_t*)LSrc;
            Dst = (uint8_t*)LDst;

            for (; X < W; X++) {
                *Dst++ = *Src++;
            }

            Dst += DMod;
            Src += SMod;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static inline void BitBlit(PSrvBitmap* sbm, PSrvBitmap* dbm, int sx, int sy, int dx, int dy, int w, int h)
{
    int Smod, Dmod;
    int BytesPerPix = 1;

    int InPtr, OutPtr;

    int nBitsPerPix = BitsPerPixel(dbm->m_ColorSpace);

    if (nBitsPerPix == 15) {
        BytesPerPix = 2;
    } else {
        BytesPerPix = nBitsPerPix / 8;
    }

    sx *= BytesPerPix;
    dx *= BytesPerPix;
    w *= BytesPerPix;

    if (sx >= dx)
    {
        if (sy >= dy)
        {
            Smod = sbm->m_BytesPerLine - w;
            Dmod = dbm->m_BytesPerLine - w;
            InPtr = sy * sbm->m_BytesPerLine + sx;
            OutPtr = dy * dbm->m_BytesPerLine + dx;

            Blit(sbm->m_Raster + InPtr, dbm->m_Raster + OutPtr, Smod, Dmod, w, h, false);
        }
        else
        {
            Smod = -sbm->m_BytesPerLine - w;
            Dmod = -dbm->m_BytesPerLine - w;
            InPtr = ((sy + h - 1) * sbm->m_BytesPerLine) + sx;
            OutPtr = ((dy + h - 1) * dbm->m_BytesPerLine) + dx;

            Blit(sbm->m_Raster + InPtr, dbm->m_Raster + OutPtr, Smod, Dmod, w, h, false);
        }
    }
    else
    {
        if (sy > dy)
        {
            Smod = -(sbm->m_BytesPerLine + w);
            Dmod = -(dbm->m_BytesPerLine + w);
            InPtr = (sy * sbm->m_BytesPerLine) + sx + w - 1;
            OutPtr = (dy * dbm->m_BytesPerLine) + dx + w - 1;
            Blit(sbm->m_Raster + InPtr, dbm->m_Raster + OutPtr, Smod, Dmod, w, h, true);
        }
        else
        {
            Smod = sbm->m_BytesPerLine - w;
            Dmod = dbm->m_BytesPerLine - w;
            InPtr = (sy + h - 1) * sbm->m_BytesPerLine + sx + w - 1;
            OutPtr = (dy + h - 1) * dbm->m_BytesPerLine + dx + w - 1;
            Blit(sbm->m_Raster + InPtr, dbm->m_Raster + OutPtr, Smod, Dmod, w, h, true);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static inline void blit_convert_copy(PSrvBitmap* dstBitmap, PSrvBitmap* srcBitmap, const PIRect& srcRect, const PIPoint& dstPos)
{
    switch (srcBitmap->m_ColorSpace)
    {
        case PEColorSpace::CMAP8:
        {
            uint8_t* src = RAS_OFFSET8(srcBitmap->m_Raster, srcRect.left, srcRect.top, srcBitmap->m_BytesPerLine);

            const int srcModulo = srcBitmap->m_BytesPerLine - srcRect.Width();

            switch (dstBitmap->m_ColorSpace)
            {
                case PEColorSpace::RGB15:
                case PEColorSpace::RGBA15:
                {
                    uint16_t* dst = RAS_OFFSET16(dstBitmap->m_Raster, dstPos.x, dstPos.y, dstBitmap->m_BytesPerLine);

                    const int dstModulo = dstBitmap->m_BytesPerLine - srcRect.Width() * 2;

                    for (int y = srcRect.top; y <= srcRect.bottom; ++y)
                    {
                        for (int x = srcRect.left; x <= srcRect.right; ++x) {
                            *dst++ = PDisplayDriver::GetPaletteEntry(*src++).GetColor15();
                        }
                        src += srcModulo;
                        dst = (uint16_t*)(((uint8_t*)dst) + dstModulo);
                    }
                    break;
                }
                case PEColorSpace::RGB16:
                {
                    uint16_t* dst = RAS_OFFSET16(dstBitmap->m_Raster, dstPos.x, dstPos.y, dstBitmap->m_BytesPerLine);

                    const int dstModulo = dstBitmap->m_BytesPerLine - srcRect.Width() * 2;

                    for (int y = srcRect.top; y <= srcRect.bottom; ++y)
                    {
                        for (int x = srcRect.left; x <= srcRect.right; ++x) {
                            *dst++ = PDisplayDriver::GetPaletteEntry(*src++).GetColor16();
                        }
                        src += srcModulo;
                        dst = (uint16_t*)(((uint8_t*)dst) + dstModulo);
                    }
                    break;
                }
                case PEColorSpace::RGB32:
                case PEColorSpace::RGBA32:
                {
                    uint32_t* dst = RAS_OFFSET32(dstBitmap->m_Raster, dstPos.x, dstPos.y, dstBitmap->m_BytesPerLine);
                    const int dstModulo = dstBitmap->m_BytesPerLine - srcRect.Width() * 4;

                    for (int y = srcRect.top; y <= srcRect.bottom; ++y)
                    {
                        for (int x = srcRect.left; x <= srcRect.right; ++x) {
                            *dst++ = PDisplayDriver::GetPaletteEntry(*src++).GetColor32();
                        }
                        src += srcModulo;
                        dst = (uint32_t*)(((uint8_t*)dst) + dstModulo);
                    }
                    break;
                }
                default:
                    break;
            }
            break;
        }
        case PEColorSpace::RGB15:
        case PEColorSpace::RGBA15:
        {
            uint16_t* src = RAS_OFFSET16(srcBitmap->m_Raster, srcRect.left, srcRect.top, srcBitmap->m_BytesPerLine);
            const int srcModulo = srcBitmap->m_BytesPerLine - srcRect.Width() * 2;

            switch (dstBitmap->m_ColorSpace)
            {
                case PEColorSpace::RGB16:
                {
                    uint16_t* dst = RAS_OFFSET16(dstBitmap->m_Raster, dstPos.x, dstPos.y, dstBitmap->m_BytesPerLine);
                    const int dstModulo = dstBitmap->m_BytesPerLine - srcRect.Width() * 2;

                    for (int y = srcRect.top; y <= srcRect.bottom; ++y)
                    {
                        for (int x = srcRect.left; x <= srcRect.right; ++x) {
                            *dst++ = PColor::FromRGB15(*src++).GetColor16();
                        }
                        src = (uint16_t*)(((uint8_t*)src) + srcModulo);
                        dst = (uint16_t*)(((uint8_t*)dst) + dstModulo);
                    }
                    break;
                }
                case PEColorSpace::RGB32:
                case PEColorSpace::RGBA32:
                {
                    uint32_t* dst = RAS_OFFSET32(dstBitmap->m_Raster, dstPos.x, dstPos.y, dstBitmap->m_BytesPerLine);
                    const int dstModulo = dstBitmap->m_BytesPerLine - srcRect.Width() * 4;

                    for (int y = srcRect.top; y <= srcRect.bottom; ++y)
                    {
                        for (int x = srcRect.left; x <= srcRect.right; ++x) {
                            *dst++ = PColor::FromRGB15(*src++).GetColor32();
                        }
                        src = (uint16_t*)(((uint8_t*)src) + srcModulo);
                        dst = (uint32_t*)(((uint8_t*)dst) + dstModulo);
                    }
                    break;
                }
                default:
                    break;
            }
            break;
        }
        case PEColorSpace::RGB16:
        {
            uint16_t* src = RAS_OFFSET16(srcBitmap->m_Raster, srcRect.left, srcRect.top, srcBitmap->m_BytesPerLine);
            const int srcModulo = srcBitmap->m_BytesPerLine - srcRect.Width() * 2;

            switch (dstBitmap->m_ColorSpace)
            {
                case PEColorSpace::RGB15:
                case PEColorSpace::RGBA15:
                {
                    uint16_t* dst = RAS_OFFSET16(dstBitmap->m_Raster, dstPos.x, dstPos.y, dstBitmap->m_BytesPerLine);
                    const int dstModulo = dstBitmap->m_BytesPerLine - srcRect.Width() * 2;

                    for (int y = srcRect.top; y <= srcRect.bottom; ++y)
                    {
                        for (int x = srcRect.left; x <= srcRect.right; ++x) {
                            *dst++ = PColor::FromRGB16(*src++).GetColor15();
                        }
                        src = (uint16_t*)(((uint8_t*)src) + srcModulo);
                        dst = (uint16_t*)(((uint8_t*)dst) + dstModulo);
                    }
                    break;
                }
                case PEColorSpace::RGB32:
                case PEColorSpace::RGBA32:
                {
                    uint32_t* dst = RAS_OFFSET32(dstBitmap->m_Raster, dstPos.x, dstPos.y, dstBitmap->m_BytesPerLine);
                    const int dstModulo = dstBitmap->m_BytesPerLine - srcRect.Width() * 4;

                    for (int y = srcRect.top; y <= srcRect.bottom; ++y)
                    {
                        for (int x = srcRect.left; x <= srcRect.right; ++x) {
                            *dst++ = PColor::FromRGB16(*src++).GetColor32();
                        }
                        src = (uint16_t*)(((uint8_t*)src) + srcModulo);
                        dst = (uint32_t*)(((uint8_t*)dst) + dstModulo);
                    }
                    break;
                }
                default:
                    break;
            }
            break;
        }
        case PEColorSpace::RGB32:
        case PEColorSpace::RGBA32:
        {
            uint32_t* src = RAS_OFFSET32(srcBitmap->m_Raster, srcRect.left, srcRect.top, srcBitmap->m_BytesPerLine);
            const int srcModulo = srcBitmap->m_BytesPerLine - srcRect.Width() * 4;

            switch (dstBitmap->m_ColorSpace)
            {
                case PEColorSpace::RGB16:
                {
                    uint16_t* dst = RAS_OFFSET16(dstBitmap->m_Raster, dstPos.x, dstPos.y, dstBitmap->m_BytesPerLine);
                    const int dstModulo = dstBitmap->m_BytesPerLine - srcRect.Width() * 2;

                    for (int y = srcRect.top; y <= srcRect.bottom; ++y)
                    {
                        for (int x = srcRect.left; x <= srcRect.right; ++x) {
                            *dst++ = PColor::FromRGB32A(*src++).GetColor16();
                        }
                        src = (uint32_t*)(((uint8_t*)src) + srcModulo);
                        dst = (uint16_t*)(((uint8_t*)dst) + dstModulo);
                    }
                    break;
                }
                case PEColorSpace::RGB15:
                {
                    uint16_t* dst = RAS_OFFSET16(dstBitmap->m_Raster, dstPos.x, dstPos.y, dstBitmap->m_BytesPerLine);
                    const int dstModulo = dstBitmap->m_BytesPerLine - srcRect.Width() * 2;

                    for (int y = srcRect.top; y <= srcRect.bottom; ++y)
                    {
                        for (int x = srcRect.left; x <= srcRect.right; ++x) {
                            *dst++ = PColor::FromRGB32A(*src++).GetColor15();
                        }
                        src = (uint32_t*)(((uint8_t*)src) + srcModulo);
                        dst = (uint16_t*)(((uint8_t*)dst) + dstModulo);
                    }
                    break;
                }
                default:
                    break;
            }
            break;
        }
        default:
            p_system_log<PLogSeverity::ERROR>(LogCategoryAppServer, "blit_convert_copy() unknown src color space {}.", int(srcBitmap->m_ColorSpace));
            break;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static inline void blit_convert_over(PSrvBitmap* dstBitmap, PSrvBitmap* srcBitmap, const PIRect& srcRect, const PIPoint& dstPos)
{
    switch (srcBitmap->m_ColorSpace)
    {
        case PEColorSpace::CMAP8:
        {
            uint8_t* src = RAS_OFFSET8(srcBitmap->m_Raster, srcRect.left, srcRect.top, srcBitmap->m_BytesPerLine);

            const int srcModulo = srcBitmap->m_BytesPerLine - srcRect.Width();

            switch (dstBitmap->m_ColorSpace)
            {
                case PEColorSpace::RGB15:
                case PEColorSpace::RGBA15:
                {
                    uint16_t* dst = RAS_OFFSET16(dstBitmap->m_Raster, dstPos.x, dstPos.y, dstBitmap->m_BytesPerLine);

                    const int dstModulo = dstBitmap->m_BytesPerLine - srcRect.Width() * 2;

                    for (int y = srcRect.top; y <= srcRect.bottom; ++y)
                    {
                        for (int x = srcRect.left; x <= srcRect.right; ++x)
                        {
                            uint8_t nPix = *src++;
                            if (nPix != PTransparentColors::CMAP8) {
                                *dst = PDisplayDriver::GetPaletteEntry(nPix).GetColor15();
                            }
                            dst++;
                        }
                        src += srcModulo;
                        dst = (uint16_t*)(((uint8_t*)dst) + dstModulo);
                    }
                    break;
                }
                case PEColorSpace::RGB16:
                {
                    uint16_t* dst = RAS_OFFSET16(dstBitmap->m_Raster, dstPos.x, dstPos.y, dstBitmap->m_BytesPerLine);
                    const int dstModulo = dstBitmap->m_BytesPerLine - srcRect.Width() * 2;

                    for (int y = srcRect.top; y <= srcRect.bottom; ++y)
                    {
                        for (int x = srcRect.left; x <= srcRect.right; ++x)
                        {
                            uint8_t nPix = *src++;
                            if (nPix != PTransparentColors::CMAP8) {
                                *dst = PDisplayDriver::GetPaletteEntry(nPix).GetColor16();
                            }
                            dst++;
                        }
                        src += srcModulo;
                        dst = (uint16_t*)(((uint8_t*)dst) + dstModulo);
                    }
                    break;
                }
                case PEColorSpace::RGB32:
                case PEColorSpace::RGBA32:
                {
                    uint32_t* dst = RAS_OFFSET32(dstBitmap->m_Raster, dstPos.x, dstPos.y, dstBitmap->m_BytesPerLine);
                    const int dstModulo = dstBitmap->m_BytesPerLine - srcRect.Width() * 4;

                    for (int y = srcRect.top; y <= srcRect.bottom; ++y)
                    {
                        for (int x = srcRect.left; x <= srcRect.right; ++x)
                        {
                            uint8_t nPix = *src++;
                            if (nPix != PTransparentColors::CMAP8) {
                                *dst = PDisplayDriver::GetPaletteEntry(nPix).GetColor32();
                            }
                            dst++;
                        }
                        src += srcModulo;
                        dst = (uint32_t*)(((uint8_t*)dst) + dstModulo);
                    }
                    break;
                }
                default:
                    p_system_log<PLogSeverity::ERROR>(LogCategoryAppServer, "blit_convert_over() unknown dst colorspace for 8 bit src {}.", int(dstBitmap->m_ColorSpace));
                    break;
            }
            break;
        }
        case PEColorSpace::RGB15:
        case PEColorSpace::RGBA15:
        {
            uint16_t* src = RAS_OFFSET16(srcBitmap->m_Raster, srcRect.left, srcRect.top, srcBitmap->m_BytesPerLine);
            const int srcModulo = srcBitmap->m_BytesPerLine - srcRect.Width() * 2;

            switch (dstBitmap->m_ColorSpace)
            {
                case PEColorSpace::RGB16:
                {
                    uint16_t* dst = RAS_OFFSET16(dstBitmap->m_Raster, dstPos.x, dstPos.y, dstBitmap->m_BytesPerLine);
                    const int dstModulo = dstBitmap->m_BytesPerLine - srcRect.Width() * 2;

                    for (int y = srcRect.top; y <= srcRect.bottom; ++y)
                    {
                        for (int x = srcRect.left; x <= srcRect.right; ++x) {
                            *dst++ = PColor::FromRGB15(*src++).GetColor16();
                        }
                        src = (uint16_t*)(((uint8_t*)src) + srcModulo);
                        dst = (uint16_t*)(((uint8_t*)dst) + dstModulo);
                    }
                    break;
                }
                case PEColorSpace::RGB32:
                case PEColorSpace::RGBA32:
                {
                    uint32_t* dst = RAS_OFFSET32(dstBitmap->m_Raster, dstPos.x, dstPos.y, dstBitmap->m_BytesPerLine);

                    const int dstModulo = dstBitmap->m_BytesPerLine - srcRect.Width() * 4;

                    for (int y = srcRect.top; y <= srcRect.bottom; ++y)
                    {
                        for (int x = srcRect.left; x <= srcRect.right; ++x) {
                            *dst++ = PColor::FromRGB15(*src++).GetColor32();
                        }
                        src = (uint16_t*)(((uint8_t*)src) + srcModulo);
                        dst = (uint32_t*)(((uint8_t*)dst) + dstModulo);
                    }
                    break;
                }
                default:
                    break;
            }
            break;
        }
        case PEColorSpace::RGB16:
        {
            uint16_t* src = RAS_OFFSET16(srcBitmap->m_Raster, srcRect.left, srcRect.top, srcBitmap->m_BytesPerLine);
            const int srcModulo = srcBitmap->m_BytesPerLine - srcRect.Width() * 2;

            switch (dstBitmap->m_ColorSpace)
            {
                case PEColorSpace::RGB15:
                case PEColorSpace::RGBA15:
                {
                    uint16_t* dst = RAS_OFFSET16(dstBitmap->m_Raster, dstPos.x, dstPos.y, dstBitmap->m_BytesPerLine);
                    const int dstModulo = dstBitmap->m_BytesPerLine - srcRect.Width() * 2;

                    for (int y = srcRect.top; y <= srcRect.bottom; ++y)
                    {
                        for (int x = srcRect.left; x <= srcRect.right; ++x) {
                            *dst++ = PColor::FromRGB16(*src++).GetColor15();
                        }
                        src = (uint16_t*)(((uint8_t*)src) + srcModulo);
                        dst = (uint16_t*)(((uint8_t*)dst) + dstModulo);
                    }
                    break;
                }
                case PEColorSpace::RGB32:
                case PEColorSpace::RGBA32:
                {
                    uint32_t* dst = RAS_OFFSET32(dstBitmap->m_Raster, dstPos.x, dstPos.y, dstBitmap->m_BytesPerLine);
                    const int dstModulo = dstBitmap->m_BytesPerLine - srcRect.Width() * 4;

                    for (int y = srcRect.top; y <= srcRect.bottom; ++y)
                    {
                        for (int x = srcRect.left; x <= srcRect.right; ++x) {
                            *dst++ = PColor::FromRGB16(*src++).GetColor32();
                        }
                        src = (uint16_t*)(((uint8_t*)src) + srcModulo);
                        dst = (uint32_t*)(((uint8_t*)dst) + dstModulo);
                    }
                    break;
                }
                default:
                    break;
            }
            break;
        }
        case PEColorSpace::RGB32:
        case PEColorSpace::RGBA32:
        {
            uint32_t* src = RAS_OFFSET32(srcBitmap->m_Raster, srcRect.left, srcRect.top, srcBitmap->m_BytesPerLine);
            const int srcModulo = srcBitmap->m_BytesPerLine - srcRect.Width() * 4;

            switch (dstBitmap->m_ColorSpace)
            {
                case PEColorSpace::RGB16:
                {
                    uint16_t* dst = RAS_OFFSET16(dstBitmap->m_Raster, dstPos.x, dstPos.y, dstBitmap->m_BytesPerLine);
                    const int dstModulo = dstBitmap->m_BytesPerLine - srcRect.Width() * 2;

                    for (int y = srcRect.top; y <= srcRect.bottom; ++y)
                    {
                        for (int x = srcRect.left; x <= srcRect.right; ++x)
                        {
                            uint32_t nPix = *src++;
                            if (nPix != 0xffffffff) {
                                *dst = PColor::FromRGB32A(nPix).GetColor16();
                            }
                            dst++;
                        }
                        src = (uint32_t*)(((uint8_t*)src) + srcModulo);
                        dst = (uint16_t*)(((uint8_t*)dst) + dstModulo);
                    }
                    break;
                }
                case PEColorSpace::RGB15:
                {
                    uint16_t* dst = RAS_OFFSET16(dstBitmap->m_Raster, dstPos.x, dstPos.y, dstBitmap->m_BytesPerLine);
                    const int dstModulo = dstBitmap->m_BytesPerLine - srcRect.Width() * 2;

                    for (int y = srcRect.top; y <= srcRect.bottom; ++y)
                    {
                        for (int x = srcRect.left; x <= srcRect.right; ++x)
                        {
                            uint32_t nPix = *src++;
                            if (nPix != 0xffffffff) {
                                *dst = PColor::FromRGB32A(nPix).GetColor15();
                            }
                            dst++;
                        }
                        src = (uint32_t*)(((uint8_t*)src) + srcModulo);
                        dst = (uint16_t*)(((uint8_t*)dst) + dstModulo);
                    }
                    break;
                }
                case PEColorSpace::RGB32:
                {
                    uint32_t* dst = RAS_OFFSET32(dstBitmap->m_Raster, dstPos.x, dstPos.y, dstBitmap->m_BytesPerLine);
                    const int dstModulo = dstBitmap->m_BytesPerLine - srcRect.Width() * 4;

                    for (int y = srcRect.top; y <= srcRect.bottom; ++y)
                    {
                        for (int x = srcRect.left; x <= srcRect.right; ++x)
                        {
                            uint32_t nPix = *src++;
                            if (nPix != 0xffffffff) {
                                *dst = nPix;
                            }
                            dst++;
                        }
                        src = (uint32_t*)(((uint8_t*)src) + srcModulo);
                        dst = (uint32_t*)(((uint8_t*)dst) + dstModulo);
                    }
                    break;
                }
                default:
                    break;
            }
            break;
        }
        default:
            p_system_log<PLogSeverity::ERROR>(LogCategoryAppServer, "blit_convert_over() unknown src color space {}.", int(srcBitmap->m_ColorSpace));
            break;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static inline void blit_convert_alpha(PSrvBitmap* dstBitmap, PSrvBitmap* srcBitmap, const PIRect& srcRect, const PIPoint& dstPos)
{
    uint32_t* src = RAS_OFFSET32(srcBitmap->m_Raster, srcRect.left, srcRect.top, srcBitmap->m_BytesPerLine);
    const int srcModulo = srcBitmap->m_BytesPerLine - srcRect.Width() * 4;

    switch (dstBitmap->m_ColorSpace)
    {
        case PEColorSpace::RGB16:
        {
            uint16_t* dst = RAS_OFFSET16(dstBitmap->m_Raster, dstPos.x, dstPos.y, dstBitmap->m_BytesPerLine);
            const int dstModulo = dstBitmap->m_BytesPerLine - srcRect.Width() * 2;

            for (int y = srcRect.top; y <= srcRect.bottom; ++y)
            {
                for (int x = srcRect.left; x <= srcRect.right; ++x)
                {
                    PColor sSrcColor = PColor::FromRGB32A(*src++);

                    int nAlpha = sSrcColor.GetAlpha();
                    if (nAlpha == 0xff) {
                        *dst = sSrcColor.GetColor16();
                    } else if (nAlpha != 0x00) {
                        PColor sDstColor = PColor::FromRGB16(*dst);
                        *dst = PColor(uint8_t(sDstColor.GetRed() * (256 - nAlpha) / 256   + sSrcColor.GetRed() * nAlpha / 256),
                                      uint8_t(sDstColor.GetGreen() * (256 - nAlpha) / 256 + sSrcColor.GetGreen() * nAlpha / 256),
                                      uint8_t(sDstColor.GetBlue() * (256 - nAlpha) / 256  + sSrcColor.GetBlue() * nAlpha / 256),
                                      0).GetColor16();
                    }
                    dst++;
                }
                src = (uint32_t*)(((uint8_t*)src) + srcModulo);
                dst = (uint16_t*)(((uint8_t*)dst) + dstModulo);
            }
            break;
        }
        case PEColorSpace::RGB15:
        {
            uint16_t* dst = RAS_OFFSET16(dstBitmap->m_Raster, dstPos.x, dstPos.y, dstBitmap->m_BytesPerLine);
            const int dstModulo = dstBitmap->m_BytesPerLine - srcRect.Width() * 2;

            for (int y = srcRect.top; y <= srcRect.bottom; ++y)
            {
                for (int x = srcRect.left; x <= srcRect.right; ++x)
                {
                    PColor sSrcColor = PColor::FromRGB32A(*src++);

                    int nAlpha = sSrcColor.GetAlpha();
                    if (nAlpha == 0xff) {
                        *dst = sSrcColor.GetColor15();
                    } else if (nAlpha != 0x00) {
                        PColor sDstColor = PColor::FromRGB15(*dst);
                        *dst = PColor(uint8_t(sDstColor.GetRed() * (256 - nAlpha) / 256   + sSrcColor.GetRed() * nAlpha / 256),
                                       uint8_t(sDstColor.GetGreen() * (256 - nAlpha) / 256 + sSrcColor.GetGreen() * nAlpha / 256),
                                       uint8_t(sDstColor.GetBlue() * (256 - nAlpha) / 256  + sSrcColor.GetBlue() * nAlpha / 256),
                                       0).GetColor15();
                    }
                    dst++;
                }
                src = (uint32_t*)(((uint8_t*)src) + srcModulo);
                dst = (uint16_t*)(((uint8_t*)dst) + dstModulo);
            }
            break;
        }
        case PEColorSpace::RGB32:
        {
            uint32_t* dst = RAS_OFFSET32(dstBitmap->m_Raster, dstPos.x, dstPos.y, dstBitmap->m_BytesPerLine);
            const int dstModulo = dstBitmap->m_BytesPerLine - srcRect.Width() * 4;

            for (int y = srcRect.top; y <= srcRect.bottom; ++y)
            {
                for (int x = srcRect.left; x <= srcRect.right; ++x)
                {
                    PColor sSrcColor = PColor::FromRGB32A(*src++);

                    int nAlpha = sSrcColor.GetAlpha();
                    if (nAlpha == 0xff) {
                        *dst = sSrcColor.GetColor32();
                    } else if (nAlpha != 0x00) {
                        PColor sDstColor = PColor::FromRGB32A(*dst);
                        *dst = PColor(uint8_t(sDstColor.GetRed() * (256 - nAlpha) / 256 + sSrcColor.GetRed() * nAlpha / 256),
                                       uint8_t(sDstColor.GetGreen() * (256 - nAlpha) / 256 + sSrcColor.GetGreen() * nAlpha / 256),
                                       uint8_t(sDstColor.GetBlue() * (256 - nAlpha) / 256 + sSrcColor.GetBlue() * nAlpha / 256),
                                       0).GetColor32();
                    }
                    dst++;
                }
                src = (uint32_t*)(((uint8_t*)src) + srcModulo);
                dst = (uint32_t*)(((uint8_t*)dst) + dstModulo);
            }
            break;
        }
        default:
            break;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDisplayDriver::CopyRect(PSrvBitmap* dstBitmap, PSrvBitmap* srcBitmap, PColor bgColor, PColor fgColor, const PIRect& srcRect, const PIPoint& dstPos, PDrawingMode mode)
{
    switch (mode)
    {
        case PDrawingMode::Copy:
            if (srcBitmap->m_ColorSpace == dstBitmap->m_ColorSpace)
            {
                int sx = srcRect.left;
                int sy = srcRect.top;
                int dx = dstPos.x;
                int dy = dstPos.y;
                int w = srcRect.Width();
                int h = srcRect.Height();

                BitBlit(srcBitmap, dstBitmap, sx, sy, dx, dy, w, h);
            }
            else
            {
                blit_convert_copy(dstBitmap, srcBitmap, srcRect, dstPos);
            }
            break;
        case PDrawingMode::Overlay:
            blit_convert_over(dstBitmap, srcBitmap, srcRect, dstPos);
            break;
        case PDrawingMode::Blend:
            if (srcBitmap->m_ColorSpace == PEColorSpace::RGB32) {
                blit_convert_alpha(dstBitmap, srcBitmap, srcRect, dstPos);
            } else {
                blit_convert_over(dstBitmap, srcBitmap, srcRect, dstPos);
            }
            break;
        default:
            break;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDisplayDriver::ScaleRect(PSrvBitmap* dstBitmap, PSrvBitmap* srcBitmap, PColor bgColor, PColor fgColor, const PIRect& srcOrigRect, const PIRect& dstOrigRect, const PRect& srcRect, const PIRect& dstRect, PDrawingMode mode)
{

}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

#if 0
void DisplayDriver::BltBitmapMask(SrvBitmap* dstBitmap, SrvBitmap* srcBitMap, const PColor& sHighColor, const PColor& sLowColor, IRect   srcRect, IPoint dstPos)
{
    int  DX = dstPos.x;
    int  DY = dstPos.y;

    int  SX = srcRect.left;
    int  SY = srcRect.top;

    int  W = srcRect.Width() + 1;
    int  H = srcRect.Height() + 1;

    uint32_t Fg = ConvertColor32(sHighColor, dstBitmap->m_ColorSpace);
    uint32_t Bg = ConvertColor32(sLowColor, dstBitmap->m_ColorSpace);

    char    CB;
    int X, Y, SBit;
    int BytesPerPix = 1;

    uint32_t    BPR, SByte, DYoff;

    BPR = srcBitMap->m_BytesPerLine;

    BytesPerPix = BitsPerPixel(dstBitmap->m_ColorSpace) / 8;
    //  if ( dstBitmap->m_nBitsPerPixel > 8 ) BytesPerPix++;
    //  if ( dstBitmap->m_nBitsPerPixel > 16 )    BytesPerPix++;
    //  if ( dstBitmap->m_nBitsPerPixel > 24 )    BytesPerPix++;
    /*
      if ( BytesPerPix > 1 )
      {
      Fg = PenToRGB( DBM->ColorMap, Fg );
      Bg = PenToRGB( DBM->ColorMap, Bg );
      }
      */
    switch (BytesPerPix)
    {
        case 1:
        case 2:
            DYoff = DY * dstBitmap->m_BytesPerLine / BytesPerPix;
            break;
        case 3:
            DYoff = DY * dstBitmap->m_BytesPerLine;
            break;
        default:
            DYoff = DY * dstBitmap->m_BytesPerLine;
            __assertw(0);
            break;
    }

    for (Y = 0; Y < H; Y++)
    {
        SByte = (SY * BPR) + (SX / 8);
        CB = srcBitMap->m_Raster[SByte++];
        SBit = 7 - (SX % 8);

        switch (BytesPerPix)
        {
            case 1:
                for (X = 0; X < W; X++)
                {
                    if (CB & (1L << SBit))
                    {
                        dstBitmap->m_Raster[DYoff + DX] = Fg;
                    } else
                    {
                        dstBitmap->m_Raster[DYoff + DX] = Bg;
                    }
                    SX++;
                    DX++;
                    if (!SBit)
                    {
                        SBit = 8;
                        CB = srcBitMap->m_Raster[SByte++];
                    }
                    SBit--;
                }
                break;
            case 2:
                for (X = 0; X < W; X++)
                {
                    if (CB & (1L << SBit))
                    {
                        ((uint16_t*)dstBitmap->m_Raster)[DYoff + DX] = Fg;
                    } else
                    {
                        ((uint16_t*)dstBitmap->m_Raster)[DYoff + DX] = Bg;
                    }
                    SX++;
                    DX++;
                    if (!SBit)
                    {
                        SBit = 8;
                        CB = srcBitMap->m_Raster[SByte++];
                    }
                    SBit--;
                }
                break;
            case 3:
                for (X = 0; X < W; X++)
                {
                    if (CB & (1L << SBit))
                    {
                        dstBitmap->m_Raster[DYoff + DX * 3] = Fg & 0xff;
                        ((uint16_t*)&dstBitmap->m_Raster[DYoff + DX * 3 + 1])[0] = Fg >> 8;
                    } else
                    {
                        dstBitmap->m_Raster[DYoff + DX * 3] = Bg & 0xff;
                        ((uint16_t*)&dstBitmap->m_Raster[DYoff + DX * 3 + 1])[0] = Bg >> 8;
                    }
                    SX++;
                    DX++;
                    if (!SBit)
                    {
                        SBit = 8;
                        CB = srcBitMap->m_Raster[SByte++];
                    }
                    SBit--;
                }
                break;
        }
        SX -= W;
        DX -= W;

        SY++;
        DY++;

        if (BytesPerPix == 2)
            DYoff += dstBitmap->m_BytesPerLine / 2;
        else
            DYoff += dstBitmap->m_BytesPerLine;
    }
}
#endif

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDisplayDriver::FillCircle(PSrvBitmap* bitmap, const PIRect& clipRect, const PIPoint& center, int32_t radius, const PColor& color, PDrawingMode mode)
{
    const float radiusSqr = float(radius * radius);

    SetFgColor(color);

    for (int32_t y = radius; y > 0; --y)
    {
        const float fy = float(y) + 0.5f;
        const int32_t deltaX = int32_t(std::round(std::sqrt(std::max(0.0f, radiusSqr - fy * fy))));

        PIRect topRect(center.x - deltaX, center.y - y, center.x + deltaX + 1, center.y - y + 1);
        topRect &= clipRect;

        if (topRect.IsValid()) {
            FillRect(bitmap, topRect);
        }

        PIRect bottomRect(center.x - deltaX, center.y + y, center.x + deltaX + 1, center.y + y + 1);
        bottomRect &= clipRect;

        if (bottomRect.IsValid()) {
            FillRect(bitmap, bottomRect);
        }
    }
    PIRect midRect(center.x - radius, center.y, center.x + radius + 1, center.y + 1);
    midRect &= clipRect;

    if (midRect.IsValid()) {
        FillRect(bitmap, midRect);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

uint32_t PDisplayDriver::WriteString(PSrvBitmap* bitmap, const PIPoint& position, const char* string, size_t strLength, const PIRect& clipRect, PColor colorBg, PColor colorFg, PFontID fontID)
{
    return position.x;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

//bool DisplayDriver::RenderGlyph(SrvBitmap* bitmap, Glyph* pcGlyph, const IPoint& cPos, const IRect& cClipRect, uint32_t* anPalette)
//{
//    IRect cBounds = pcGlyph->m_cBounds + cPos;
//    IRect cRect = cBounds & cClipRect;
//
//    if (cRect.IsValid())
//    {
//        int   sx = cRect.left - cBounds.left;
//        int   sy = cRect.top - cBounds.top;
//
//        int   nWidth = cRect.Width();
//        int   nHeight = cRect.Height();
//
//        const int   srcModulo = pcGlyph->m_BytesPerLine - nWidth;
//        const int   dstModulo = bitmap->m_BytesPerLine / 2 - nWidth;
//
//        uint8_t* src = pcGlyph->m_Raster + sx + sy * pcGlyph->m_BytesPerLine;
//        uint16_t* dst = (uint16_t*)bitmap->m_Raster + cRect.left + (cRect.top * bitmap->m_BytesPerLine / 2);
//
//        for (int y = 0; y < nHeight; ++y)
//        {
//            for (int x = 0; x < nWidth; ++x)
//            {
//                int nPix = *src++;
//                if (nPix > 0) {
//                    *dst = anPalette[nPix - 1];
//                }
//                dst++;
//            }
//            src += srcModulo;
//            dst += dstModulo;
//        }
//    }
//    return true;
//}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

//bool DisplayDriver::RenderGlyph(SrvBitmap* bitmap, Glyph* pcGlyph, const IPoint& cPos, const IRect& cClipRect, const PColor& sFgColor)
//{
//    IRect cBounds = pcGlyph->m_cBounds + cPos;
//    IRect cRect = cBounds & cClipRect;
//
//    if (cRect.IsValid() == false) {
//        return(false);
//    }
//    int   sx = cRect.left - cBounds.left;
//    int   sy = cRect.top - cBounds.top;
//
//    int   nWidth = cRect.Width();
//    int   nHeight = cRect.Height();
//
//    const int   srcModulo = pcGlyph->m_BytesPerLine - nWidth;
//
//    uint8_t* src = pcGlyph->m_Raster + sx + sy * pcGlyph->m_BytesPerLine;
//
//    PColor sCurCol;
//    PColor sBgColor;
//
//    if (bitmap->m_ColorSpace == ColorSpace::RGB16) {
//        const int   dstModulo = bitmap->m_BytesPerLine / 2 - nWidth;
//        uint16_t* dst = (uint16_t*)bitmap->m_Raster + cRect.left + (cRect.top * bitmap->m_BytesPerLine / 2);
//
//        int nFgClut = sFgColor.GetColor16();
//
//        for (int y = 0; y < nHeight; ++y) {
//            for (int x = 0; x < nWidth; ++x) {
//                int nAlpha = *src++;
//
//                if (nAlpha > 0) {
//                    if (nAlpha == 4) {
//                        *dst = nFgClut;
//                    } else {
//                        int   nClut = *dst;
//
//                        sBgColor = PColor::FromRGB16(nClut);
//
//                        sCurCol.GetRed()   = sBgColor.GetRed()   + (sFgColor.GetRed()   - sBgColor.GetRed()) * nAlpha / 4;
//                        sCurCol.GetGreen() = sBgColor.GetGreen() + (sFgColor.GetGreen() - sBgColor.GetGreen()) * nAlpha / 4;
//                        sCurCol.GetBlue()  = sBgColor.GetBlue()  + (sFgColor.GetBlue()  - sBgColor.GetBlue()) * nAlpha / 4;
//                        *dst = sCurCol.GetColor16();
//                    }
//                }
//                dst++;
//            }
//            src += srcModulo;
//            dst += dstModulo;
//        }
//    } else if (bitmap->m_ColorSpace == ColorSpace::RGB32) {
//        const int   dstModulo = bitmap->m_BytesPerLine / 4 - nWidth;
//        uint32_t* dst = (uint32_t*)bitmap->m_Raster + cRect.left + (cRect.top * bitmap->m_BytesPerLine / 4);
//
//        int nFgClut = sFgColor.GetColor32();
//
//        for (int y = 0; y < nHeight; ++y) {
//            for (int x = 0; x < nWidth; ++x) {
//                int nAlpha = *src++;
//
//                if (nAlpha > 0) {
//                    if (nAlpha == 4) {
//                        *dst = nFgClut;
//                    } else {
//                        int   nClut = *dst;
//
//                        sBgColor = PColor::FromRGB32A(nClut);
//
//                        sCurCol.GetRed   = sBgColor.GetRed()   + (sFgColor.GetRed()   - sBgColor.GetRed()) * nAlpha / 4;
//                        sCurCol.GetGreen = sBgColor.GetGreen() + (sFgColor.GetGreen() - sBgColor.GetGreen()) * nAlpha / 4;
//                        sCurCol.GetBlue  = sBgColor.GetBlue()  + (sFgColor.GetBlue()  - sBgColor.GetBlue()) * nAlpha / 4;
//
//                        *dst = sCurCol.GetColor32();
//                    }
//                }
//                dst++;
//            }
//            src += srcModulo;
//            dst += dstModulo;
//        }
//    }
//    return true;
//}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

float PDisplayDriver::GetFontHeight(PFontID fontID) const
{
    const FONT_INFO* font = GetFontDesc(fontID);
    return (font != nullptr) ? float(font->heightPages) : 0.0f;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

float PDisplayDriver::GetStringWidth(PFontID fontID, const char* string, size_t length) const
{
    const FONT_INFO* font = GetFontDesc(fontID);

    if (font != nullptr)
    {
        float width = 0.0f;

        while (length > 0)
        {
            int charLen = utf8_char_length(*string);
            if (charLen > length) {
                break;
            }
            uint32_t character = utf8_to_unicode(string);
            string += charLen;
            length -= charLen;

            if (character < font->startChar || character > font->endChar) continue;

            const FONT_CHAR_INFO* charInfo = font->charInfo + character - font->startChar;
            width += float(charInfo->widthBits);
            if (length != 0) {
                width += float(CHARACTER_SPACING);
            }
        }
        return width;
    }
    return 0.0f;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t PDisplayDriver::GetStringLength(PFontID fontID, const char* string, size_t length, float width, bool includeLast)
{
    const FONT_INFO* font = GetFontDesc(fontID);

    if (font == nullptr) {
        return 0;
    }
    size_t strLen = 0;

    while (length > 0)
    {
        int charLen = utf8_char_length(*string);
        if (charLen > length) {
            break;
        }
        uint32_t character = utf8_to_unicode(string);
        
        float advance = 0.0f;
        if (character >= font->startChar && character <= font->endChar)
        {
            const FONT_CHAR_INFO* charInfo = font->charInfo + character - font->startChar;
            advance = float(charInfo->widthBits + CHARACTER_SPACING);
        }
        if (width < advance)
        {
            if (includeLast) {
                strLen += charLen;
            }
            break;
        }
        string += charLen;
        length -= charLen;
        strLen += charLen;
        width -= advance;
    }
    return strLen;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

const FONT_INFO* PDisplayDriver::GetFontDesc(PFontID fontID) const
{
    switch (fontID)
    {
        case PFontID::e_FontSmall:
        case PFontID::e_FontNormal:
            return &sansSerif_14ptFontInfo;
        case PFontID::e_FontLarge:
            return &sansSerif_20ptFontInfo;
        case PFontID::e_Font7Seg:
            return &sansSerif_72ptFontInfo;
        case PFontID::e_FontCount:
            return nullptr;
    }
    return nullptr;
}
