// This file is part of PadOS.
//
// Copyright (C) 2018-2026 Kurt Skauen <http://kavionic.com/>
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
// Created: 19.03.2018 20:56:30

#include <System/Platform.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <string.h>

#include <ApplicationServer/ApplicationServer.h>
#include <ApplicationServer/DisplayDriver.h>
#include <ApplicationServer/ServerBitmap.h>
#include <Utils/Utils.h>

#include "ServerView.h"


static int g_ServerViewCount = 0;

namespace
{
enum class TriangleClipEdge
{
    Left,
    Right,
    Top,
    Bottom
};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool IsInsideTriangleClipEdge(const PPoint& point, const PRect& clipRect, TriangleClipEdge edge)
{
    switch (edge)
    {
        case TriangleClipEdge::Left:
            return point.x >= clipRect.left;
        case TriangleClipEdge::Right:
            return point.x <= clipRect.right;
        case TriangleClipEdge::Top:
            return point.y >= clipRect.top;
        case TriangleClipEdge::Bottom:
            return point.y <= clipRect.bottom;
        default:
            return false;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PPoint IntersectTriangleClipEdge(const PPoint& startPoint, const PPoint& endPoint, const PRect& clipRect, TriangleClipEdge edge)
{
    const PPoint delta = endPoint - startPoint;

    switch (edge)
    {
        case TriangleClipEdge::Left:
        {
            const float ratio = (clipRect.left - startPoint.x) / delta.x;
            return PPoint(clipRect.left, startPoint.y + delta.y * ratio);
        }
        case TriangleClipEdge::Right:
        {
            const float ratio = (clipRect.right - startPoint.x) / delta.x;
            return PPoint(clipRect.right, startPoint.y + delta.y * ratio);
        }
        case TriangleClipEdge::Top:
        {
            const float ratio = (clipRect.top - startPoint.y) / delta.y;
            return PPoint(startPoint.x + delta.x * ratio, clipRect.top);
        }
        case TriangleClipEdge::Bottom:
        {
            const float ratio = (clipRect.bottom - startPoint.y) / delta.y;
            return PPoint(startPoint.x + delta.x * ratio, clipRect.bottom);
        }
        default:
            return endPoint;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t ClipTrianglePolygonEdge(
    const std::array<PPoint, 8>& inputPoints,
    size_t inputPointCount,
    std::array<PPoint, 8>& outputPoints,
    const PRect& clipRect,
    TriangleClipEdge edge)
{
    size_t outputPointCount = 0;

    if (inputPointCount == 0) {
        return 0;
    }

    PPoint startPoint = inputPoints[inputPointCount - 1];
    bool startInside = IsInsideTriangleClipEdge(startPoint, clipRect, edge);

    for (size_t pointIndex = 0; pointIndex < inputPointCount; ++pointIndex)
    {
        const PPoint endPoint = inputPoints[pointIndex];
        const bool endInside = IsInsideTriangleClipEdge(endPoint, clipRect, edge);

        if (endInside)
        {
            if (!startInside) {
                outputPoints[outputPointCount++] = IntersectTriangleClipEdge(startPoint, endPoint, clipRect, edge);
            }
            outputPoints[outputPointCount++] = endPoint;
        }
        else if (startInside)
        {
            outputPoints[outputPointCount++] = IntersectTriangleClipEdge(startPoint, endPoint, clipRect, edge);
        }
        startPoint = endPoint;
        startInside = endInside;
    }
    return outputPointCount;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t ClipTrianglePolygon(std::array<PPoint, 8>& points, size_t pointCount, const PIRect& clipRect)
{
    if (clipRect.Width() <= 0 || clipRect.Height() <= 0) {
        return 0;
    }

    std::array<PPoint, 8> scratchPoints;
    const PRect floatClipRect(float(clipRect.left), float(clipRect.top), float(clipRect.right - 1), float(clipRect.bottom - 1));

    pointCount = ClipTrianglePolygonEdge(points, pointCount, scratchPoints, floatClipRect, TriangleClipEdge::Left);
    pointCount = ClipTrianglePolygonEdge(scratchPoints, pointCount, points, floatClipRect, TriangleClipEdge::Right);
    pointCount = ClipTrianglePolygonEdge(points, pointCount, scratchPoints, floatClipRect, TriangleClipEdge::Top);
    pointCount = ClipTrianglePolygonEdge(scratchPoints, pointCount, points, floatClipRect, TriangleClipEdge::Bottom);

    return pointCount;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

struct PolylineClipVertex
{
    PPoint Point;
    float  ArcPosition;
};

size_t ClipPolylineTriangleBoundary(
    const std::array<PolylineClipVertex, 8>& inputPoints,
    size_t inputPointCount,
    std::array<PolylineClipVertex, 8>& outputPoints,
    float boundaryArc,
    bool keepAfter)
{
    size_t outputPointCount = 0;

    if (inputPointCount == 0) {
        return 0;
    }

    PolylineClipVertex startPoint = inputPoints[inputPointCount - 1];
    float startDistance = startPoint.ArcPosition - boundaryArc;
    bool startInside = keepAfter ? (startDistance >= 0.0f) : (startDistance <= 0.0f);

    for (size_t pointIndex = 0; pointIndex < inputPointCount; ++pointIndex)
    {
        const PolylineClipVertex endPoint = inputPoints[pointIndex];
        const float endDistance = endPoint.ArcPosition - boundaryArc;
        const bool endInside = keepAfter ? (endDistance >= 0.0f) : (endDistance <= 0.0f);

        if (endInside != startInside)
        {
            const float ratio = startDistance / (startDistance - endDistance);
            outputPoints[outputPointCount++] = {
                startPoint.Point + (endPoint.Point - startPoint.Point) * ratio,
                boundaryArc
            };
        }
        if (endInside) {
            outputPoints[outputPointCount++] = endPoint;
        }

        startPoint = endPoint;
        startDistance = endDistance;
        startInside = endInside;
    }
    return outputPointCount;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PIPoint RoundAndClampTrianglePoint(const PPoint& point, const PIRect& clipRect)
{
    const int x = std::clamp(int(std::round(point.x)), clipRect.left, clipRect.right - 1);
    const int y = std::clamp(int(std::round(point.y)), clipRect.top, clipRect.bottom - 1);
    return PIPoint(x, y);
}
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PServerView::PServerView(
    PSrvBitmap*          bitmap,
    const PString&      name,
    const PRect&         frame,
    const PPoint&        scrollOffset,
    PViewDockType        dockType,
    uint32_t            flags,
    int32_t             hideCount,
    PFocusKeyboardMode   focusKeyboardMode,
    PDrawingMode         drawingMode,
    float               penWidth,
    PCapStyle            capStyle,
    PJointStyle          jointStyle,
    float                miterLimit,
    const std::vector<float>& dashPattern,
    float                dashOffset,
    PFontID              fontID,
    PColor               eraseColor,
    PColor               bgColor,
    PColor               fgColor
)
    : PViewBase(name, frame, scrollOffset, flags, hideCount, penWidth, eraseColor, bgColor, fgColor, capStyle, jointStyle, miterLimit, dashPattern, dashOffset)
    , m_Bitmap(bitmap)
    , m_DockType(dockType)
    , m_FocusKeyboardMode(focusKeyboardMode)
    , m_DrawingMode(drawingMode)
{
    m_Font->Set(fontID);
    g_ServerViewCount++;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PServerView::~PServerView()
{
    g_ServerViewCount--;

    while (!m_ChildrenList.empty()) {
        RemoveChild(m_ChildrenList[m_ChildrenList.size() - 1], true);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerView::HandleAddedToParent(Ptr<PServerView> parent, size_t index)
{
    if (!parent->IsVisible()) {
        Show(false);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerView::HandleRemovedFromParent(Ptr<PServerView> parent)
{
    ApplicationServer* server = static_cast<ApplicationServer*>(GetLooper());
    if (server != nullptr) {
        server->ViewDestructed(this);
    }
    if (!parent->IsVisible()) {
        Show(true);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

    
bool PServerView::HandleMouseDown(PMouseButton button, const PPoint& position, const PMotionEvent& event)
{
    if (!IsVisible())
    {
        return false;
    }
    if (m_ClientHandle != INVALID_HANDLE && !HasFlags(PViewFlags::IgnoreMouse))
    {
        if (m_ManagerHandle != INVALID_HANDLE)
        {
            if (!p_post_to_window_manager<ASHandleMouseDown>(m_ManagerHandle, button, position, event))
            {
                p_system_log<PLogSeverity::ERROR>(LogCategoryAppServer, "ServerView::HandleMouseDown() failed to send message: {}", strerror(get_last_error()));
                return false;
            }            
        }
                    
        if (!p_post_to_remotesignal<ASHandleMouseDown>(m_ClientPort, m_ClientHandle, TimeValNanos::zero, button, position, event)) {
            p_system_log<PLogSeverity::ERROR>(LogCategoryAppServer, "ServerView::HandleMouseDown() failed to send message: {}", strerror(get_last_error()));
            return false;
        }
        ApplicationServer* server = static_cast<ApplicationServer*>(GetLooper());
        if (server !=  nullptr) {
            server->SetMouseDownView(button, ptr_tmp_cast(this));
        }
        return true;
    }
    
    for (Ptr<PServerView> child : p_reverse_ranged(m_ChildrenList))
    {
        if (child->m_Frame.DoIntersect(position))
        {
            const PPoint childPos = child->ConvertFromParent(position);
            if (child->HandleMouseDown(button, childPos, event))
            {
                return true;
            }
        }
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PServerView::HandleMouseUp(PMouseButton button, const PPoint& position, const PMotionEvent& event)
{
    if (m_ClientHandle != INVALID_HANDLE)
    {
        if (!p_post_to_remotesignal<ASHandleMouseUp>(m_ClientPort, m_ClientHandle, TimeValNanos::zero, button, position, event)) {
            p_system_log<PLogSeverity::ERROR>(LogCategoryAppServer, "ServerView::HandleMouseUp() failed to send message: {}", strerror(get_last_error()));
        }
        return true;
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PServerView::HandleMouseMove(PMouseButton button, const PPoint& position, const PMotionEvent& event)
{
    if (m_ClientHandle != INVALID_HANDLE)
    {
        if (!p_post_to_remotesignal<ASHandleMouseMove>(m_ClientPort, m_ClientHandle, TimeValNanos::zero, button, position, event)) {
            p_system_log<PLogSeverity::ERROR>(LogCategoryAppServer, "ServerView::HandleMouseMove() failed to send message: {}", strerror(get_last_error()));
        }
        return true;
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerView::AddChild(Ptr<PServerView> child, size_t index)
{
    if (index != INVALID_INDEX && index > m_ChildrenList.size()) index = m_ChildrenList.size();
    LinkChild(child, index);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerView::RemoveChild(Ptr<PServerView> child, bool removeAsHandler)
{
    UnlinkChild(child);

    if (removeAsHandler)
    {
        PLooper* const looper = child->GetLooper();
        if (looper != nullptr) {
            looper->RemoveHandler(child);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerView::RemoveThis(bool removeAsHandler)
{
    const Ptr<PServerView> parent = m_Parent.Lock();
    if (parent != nullptr)
    {
        parent->RemoveChild(ptr_tmp_cast(this), removeAsHandler);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PServerView::IsVisible() const
{
    return m_HideCount == 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerView::SetFrame(const PRect& rect, handler_id requestingClient)
{
    const PIRect intFrame(rect);
    const PIRect prevIFrame = GetIFrame();

    m_Frame = rect;
    UpdateScreenPos();

    if (intFrame == prevIFrame) {
        return;
    }

    const Ptr<PServerView> parent = m_Parent.Lock();

    if (m_HideCount == 0)
    {
        m_DeltaMove += PIPoint(intFrame.left, intFrame.top ) - PIPoint(prevIFrame.left, prevIFrame.top);
        m_DeltaSize += PIPoint(intFrame.Width(), intFrame.Height() ) - PIPoint(prevIFrame.Width(), prevIFrame.Height());

        if (parent != nullptr)
        {
            for (Ptr<PServerView> i = parent; i != nullptr; i = i->GetParent())
            {
                if ((i->m_Flags & PViewFlags::Transparent) == 0) {
                    i->SetDirtyRegFlags();
                    break;
                }                    
            }

            auto i = parent->GetChildRIterator(ptr_tmp_cast(this));
            if (i != parent->m_ChildrenList.rend())
            {
                for (++i; i != parent->m_ChildrenList.rend(); ++i)
                {
                    const Ptr<PServerView> sibling = *i;
                    PIRect siblingIFrame = sibling->GetIFrame();
                    if (siblingIFrame.DoIntersect(prevIFrame) || siblingIFrame.DoIntersect(intFrame))
                    {
                        sibling->MarkModified(prevIFrame - siblingIFrame.TopLeft());
                        sibling->MarkModified(intFrame - siblingIFrame.TopLeft());
                    }
                }                
            }
        }            
    }
    
    if (requestingClient != INVALID_HANDLE)
    {
        if (requestingClient == m_ClientHandle)
        {
            if (m_ManagerHandle != INVALID_HANDLE)
            {
                ApplicationServer* const server = static_cast<ApplicationServer*>(GetLooper());
                if (server !=  nullptr) {
                    p_post_to_window_manager<ASViewFrameChanged>(m_ManagerHandle, m_Frame);
                }                    
            }
        }
        else
        {
            if (!p_post_to_remotesignal<ASViewFrameChanged>(m_ClientPort, m_ClientHandle, TimeValNanos::zero, m_Frame)) {
                p_system_log<PLogSeverity::ERROR>(LogCategoryAppServer, "ServerView::SetFrame() failed to send message: {}", strerror(get_last_error()));
            }            
        }
    }
    
    if (parent == nullptr) {
        Invalidate();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerView::SetDrawRegion(Ptr<PRegion> region)
{
    m_DrawConstrainReg = region;

    if (m_HideCount == 0) {
        SetDirtyRegFlags();
    }
    m_DrawReg = nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerView::SetShapeRegion(Ptr<PRegion> region)
{
    m_ShapeConstrainReg = region;

    if (m_HideCount == 0)
    {
        const Ptr<PServerView> parent = m_Parent.Lock();
        if (parent != nullptr)
        {
            for (Ptr<PServerView> i = parent; i != nullptr; i = i->GetParent())
            {
                if ((i->m_Flags & PViewFlags::Transparent) == 0)
                {
                    i->SetDirtyRegFlags();
                    break;
                }                    
            }
            
            auto i = parent->GetChildRIterator(ptr_tmp_cast(this));
            if (i != parent->m_ChildrenList.rend())
            {
                const PIRect intFrame = GetIFrame();
                for (++i; i != parent->m_ChildrenList.rend(); ++i)
                {
                    const Ptr<PServerView> sibling = *i;
                    const PIRect siblingIFrame = sibling->GetIFrame();
                    if ( siblingIFrame.DoIntersect(intFrame) ) {
                        sibling->MarkModified(intFrame - siblingIFrame.TopLeft());
                    }
                }
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerView::Invalidate(const PIRect& rect)
{
    if (m_HideCount == 0)
    {
        if (m_DamageReg == nullptr) {
            m_DamageReg = ptr_new<PRegion>(rect);
        } else {
            m_DamageReg->Include(rect);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerView::Invalidate(bool reqursive)
{
    if (m_HideCount == 0)
    {
        m_DamageReg = ptr_new<PRegion>(static_cast<PIRect>(GetNormalizedBounds()));

        if (reqursive)
        {
            for (Ptr<PServerView> child : m_ChildrenList) {
                child->Invalidate(true);
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerView::InvalidateNewAreas()
{
    if (m_HideCount > 0) {
        return;
    }
    /*
    if (m_pcWindow != nullptr && (m_pcWindow->GetDesktopMask() & (1 << get_active_desktop())) == 0 ) {
        return;
    }*/
  
    if (m_HasInvalidRegs)
    {
        if ( ((m_Flags & PViewFlags::FullUpdateOnResizeH) && m_DeltaSize.x != 0) || ((m_Flags & PViewFlags::FullUpdateOnResizeV) && m_DeltaSize.y != 0) )
        {
            Invalidate(false);
        }
        else
        {
            if (m_VisibleReg != nullptr)
            {
                try
                {
                    Ptr<PRegion> region = ptr_new<PRegion>(*m_VisibleReg);
    
                    if (m_PrevVisibleReg != nullptr) {
                        region->Exclude(*m_PrevVisibleReg);
                    }
                    if (m_DamageReg == nullptr)
                    {
                        if (!region->IsEmpty()) {
                            m_DamageReg = region;
                        }
                    }
                    else
                    {
                        for (const PIRect& clip : region->m_Rects) {
                            Invalidate(clip);
                        }
                    }
                }
                catch(const std::bad_alloc&) {}
            }
        }
        m_PrevVisibleReg = nullptr;

        m_DeltaSize = PIPoint(0, 0);
        m_DeltaMove = PIPoint(0, 0);
    }
  
    for (Ptr<PServerView> child : m_ChildrenList) {
        child->InvalidateNewAreas();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

struct BlitSortCompare
{
    BlitSortCompare(const PIPoint& deltaMove) : m_DeltaMove(deltaMove) {}
        
    bool operator()(PIRect* lhs, PIRect* rhs) const
    {
        if (lhs->left >= rhs->right && lhs->right <= rhs->left)
        {
            if (m_DeltaMove.x < 0) {
                return lhs->left < rhs->left;
            } else {
                return rhs->left < lhs->left;
            }
        }
        else
        {
            if (m_DeltaMove.y < 0) {
                return lhs->top < rhs->top;
            } else {
                return rhs->top < lhs->top;
            }
        }
    }
    PIPoint m_DeltaMove;
};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerView::MoveChilds()
{
    if ( m_HideCount > 0  /*|| nullptr == m_pcBitmap */) {
        return;
    }
/*    if (m_pcWindow != nullptr && (m_pcWindow->GetDesktopMask() & (1 << get_active_desktop())) == 0) {
        return;
    }*/
  
    if (m_HasInvalidRegs)
    {
        const PIRect bounds(GetNormalizedBounds());
        const PIPoint screenPos(m_ScreenPos);
        for (Ptr<PServerView> child : m_ChildrenList)
        {
            if (child->m_DeltaMove.x == 0 && child->m_DeltaMove.y == 0) {
                continue;
            }
            if (child->m_FullReg == nullptr || child->m_PrevFullReg == nullptr) {
                continue;
            }

            const Ptr<PRegion> region = ptr_new<PRegion>(*child->m_PrevFullReg);
            region->Intersect(*child->m_FullReg);

            if (region->IsEmpty()) {
                continue;
            }

            const PIPoint cChildOffset = PIPoint(child->m_ScreenPos);
            const PIPoint cChildMove(child->m_DeltaMove);
            for (PIRect& clip : region->m_Rects)
            {
                // Transform into parents coordinate system
                clip += cChildOffset;
                assert(clip.IsValid());
            }

            std::vector<PIRect*> clipList;
            clipList.reserve(region->m_Rects.size());

            for (PIRect& clip : region->m_Rects) {
                clipList.push_back(&clip);
            }
            std::sort(clipList.begin(), clipList.end(), BlitSortCompare(cChildMove));

            for (const PIRect* clip : clipList) {
                m_Bitmap->m_Driver->CopyRect(m_Bitmap, m_Bitmap, m_BgColor, m_FgColor, *clip - cChildMove, clip->TopLeft(), PDrawingMode::Copy);
            }
        }

        // Since the parent window is shrinked before the children is moved
        // we may need to redraw the right and bottom edges.

        Ptr<PServerView> parent = m_Parent.Lock();
        if ( parent != nullptr && (m_DeltaMove.x != 0 || m_DeltaMove.y != 0) )
        {
            if (parent->m_DeltaSize.x < 0)
            {
                PIRect rect = bounds;
                const PIRect parentIFrame = parent->GetIFrame();

                rect.left = rect.right + int(parent->m_DeltaSize.x + parentIFrame.right - parentIFrame.left - GetIFrame().right);

                if (rect.IsValid()) {
                    Invalidate(rect);
                }
            }
            if (parent->m_DeltaSize.y < 0)
            {
                PIRect rect = bounds;
                const PIRect parentIFrame = parent->GetIFrame();

                rect.top = rect.bottom + int(parent->m_DeltaSize.y + parentIFrame.bottom - parentIFrame.top - GetIFrame().bottom);

                if (rect.IsValid()) {
                    Invalidate(rect);
                }
            }
        }
        m_PrevFullReg = nullptr;
    }
    for (Ptr<PServerView> child : m_ChildrenList) {
        child->MoveChilds();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerView::RebuildRegion()
{
    if (m_HideCount > 0)
    {
        if (m_VisibleReg != nullptr) {
            DeleteRegions();
        }
        return;
    }
    if (m_HasInvalidRegs)
    {
        m_DrawReg = nullptr;

        assert(m_PrevVisibleReg == nullptr);
        assert(m_PrevFullReg == nullptr);

        m_PrevVisibleReg = m_VisibleReg;
        m_PrevFullReg = m_FullReg;

        const PIPoint scrollOffset(m_ScrollOffset);

        const Ptr<PServerView> parent = m_Parent.Lock();
        if (parent == nullptr)
        {
            m_FullReg = ptr_new<PRegion>(GetIFrame());
        }
        else
        {
            const PIRect intFrame = GetIFrame();
            assert(parent->m_FullReg != nullptr);
            if (parent->m_FullReg == nullptr) {
                m_FullReg = ptr_new<PRegion>(intFrame.Bounds());
            } else {
                m_FullReg = ptr_new<PRegion>(*parent->m_FullReg, intFrame + PIPoint(parent->m_ScrollOffset), true);
            }
            if (m_ShapeConstrainReg != nullptr) {
                m_FullReg->Intersect(*m_ShapeConstrainReg);
            }
            const PIPoint topLeft(intFrame.TopLeft());
            auto i = parent->GetChildIterator(ptr_tmp_cast(this));
            if (i != parent->m_ChildrenList.end())
            {
                for (++i; i != parent->m_ChildrenList.end(); ++i) // Loop over all siblings above us.
                {
                    Ptr<PServerView> sibling = *i;
                    if (sibling->m_HideCount == 0)
                    {
                        if (sibling->GetIFrame().DoIntersect(intFrame))
                        {
                            sibling->ExcludeFromRegion(m_FullReg, -topLeft);
                        }
                    }
                }
            }
            m_FullReg->Optimize();
        }
        m_VisibleReg = ptr_new<PRegion>(*m_FullReg);

        if ((m_Flags & PViewFlags::DrawOnChildren) == 0)
        {
            bool regModified = false;
            for (Ptr<PServerView> child : m_ChildrenList)
            {
                // Remove children from child region
                if (child->ExcludeFromRegion(m_VisibleReg, scrollOffset)) {
                    regModified = true;
                }
            }
            if (regModified) {
                m_VisibleReg->Optimize();
            }
        }
    }
    for (Ptr<PServerView> child : m_ChildrenList) {
        child->RebuildRegion();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PServerView::ExcludeFromRegion(Ptr<PRegion> region, const PIPoint& offset)
{
    if (m_HideCount == 0)
    {
        if ((m_Flags & PViewFlags::Transparent) == 0)
        {
            if ( m_ShapeConstrainReg == nullptr ) {
                region->Exclude(GetIFrame() + offset);
            } else {
                region->Exclude(*m_ShapeConstrainReg, GetITopLeft() + offset);
            }
            return true;
        }
        else
        {
            bool wasModified = false;
            const PIPoint framePos = GetITopLeft();
            const PIPoint scrollOffset(m_ScrollOffset);
            for (Ptr<PServerView> child : m_ChildrenList)
            {
                if (child->ExcludeFromRegion(region, offset + framePos + scrollOffset)) {
                    wasModified = true;
                }                    
            }                
            return wasModified;
        }
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerView::ClearDirtyRegFlags()
{
    m_HasInvalidRegs = false;
    for (Ptr<PServerView> child : m_ChildrenList) {
        child->ClearDirtyRegFlags();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerView::UpdateRegions()
{
    RebuildRegion();
    MoveChilds();
    InvalidateNewAreas();

    ApplicationServer* const server = static_cast<ApplicationServer*>(GetLooper());
    if (server != nullptr && server->GetTopView() == this /*m_pcBitmap != nullptr*/ && m_DamageReg != nullptr)
    {
        if (m_Bitmap == ApplicationServer::GetScreenBitmap())
        {
            PRegion cDrawReg(*m_VisibleReg);
            cDrawReg.Intersect(*m_DamageReg);
        
            m_Bitmap->m_Driver->SetFgColor(m_EraseColor);

            const PIPoint screenPos(m_ScreenPos);
            for (const PIRect& clip : cDrawReg.m_Rects) {
                m_Bitmap->m_Driver->FillRect(m_Bitmap, clip + screenPos);
            }
        }
        m_DamageReg = nullptr;
    }
    RequestPaintIfNeeded();
    ClearDirtyRegFlags();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerView::DeleteRegions()
{
    assert(m_PrevVisibleReg == nullptr);
    assert(m_PrevFullReg == nullptr);

    m_VisibleReg      = nullptr;
    m_FullReg         = nullptr;
    m_DrawReg         = nullptr;
    m_DamageReg       = nullptr;
    m_ActiveDamageReg = nullptr;
    
    for (Ptr<PServerView> child : m_ChildrenList) {
        child->DeleteRegions();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<PRegion> PServerView::GetRegion()
{
    if ( m_HideCount > 0 ) {
        return nullptr;
    }
    if (m_IsUpdating && m_ActiveDamageReg == nullptr) {
        return nullptr;
    }

    if ( !m_IsUpdating )
    {
        if ( m_DrawConstrainReg == nullptr )
        {
            return m_VisibleReg;
        }
        else
        {
            if ( m_DrawReg == nullptr )
            {
                m_DrawReg = ptr_new<PRegion>(*m_VisibleReg);
                m_DrawReg->Intersect( *m_DrawConstrainReg );
            }
        }
    }
    else if (m_DrawReg == nullptr && m_VisibleReg != nullptr)
    {
        m_DrawReg = ptr_new<PRegion>(*m_VisibleReg);

        assert(m_ActiveDamageReg != nullptr);
        m_DrawReg->Intersect(*m_ActiveDamageReg);
        if (m_DrawConstrainReg != nullptr) {
            m_DrawReg->Intersect( *m_DrawConstrainReg );
        }
        m_DrawReg->Optimize();
    }
    assert(m_DrawReg != nullptr);
    return m_DrawReg;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerView::ToggleDepth()
{
    const Ptr<PServerView> self = ptr_tmp_cast(this);
    const Ptr<PServerView> parent = GetParent();
    if (parent != nullptr)
    {
        if (parent->m_ChildrenList[parent->m_ChildrenList.size()-1] == self)
        {
            parent->RemoveChild(self, false);
            parent->AddChild(self, 0);
        }
        else
        {
            parent->RemoveChild(self, false);
            parent->AddChild(self, INVALID_INDEX);
        }

        const Ptr<PServerView> opacParent = GetOpacParent(parent, nullptr);
        assert(opacParent != nullptr);
        opacParent->SetDirtyRegFlags();
    
        const PIRect intFrame = GetIFrame();
        for (Ptr<PServerView> sibling : *parent)
        {
            const PIRect siblingIFrame = sibling->GetIFrame();
            if (siblingIFrame.DoIntersect(intFrame)) {
                sibling->MarkModified(intFrame - siblingIFrame.TopLeft());
            }
        }
        opacParent->UpdateRegions();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerView::BeginUpdate()
{
    if (m_VisibleReg != nullptr) {
        m_IsUpdating = true;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerView::EndUpdate( void )
{
    m_ActiveDamageReg = nullptr;
    m_DrawReg         = nullptr;
    
    m_IsUpdating = false;
    
    if (m_DamageReg != nullptr)
    {
        m_ActiveDamageReg = m_DamageReg;
        m_DamageReg = nullptr;
        Paint(static_cast<PRect>(m_ActiveDamageReg->GetBounds()));
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerView::Paint(const PIRect& updateRect)
{
    if ( m_HideCount > 0 || m_IsUpdating == true || m_ClientHandle == INVALID_HANDLE) {
        return;
    }
    if (!p_post_to_remotesignal<ASPaintView>(m_ClientPort, m_ClientHandle, TimeValNanos::zero, updateRect - PIPoint(m_ScrollOffset))) {
        p_system_log<PLogSeverity::ERROR>(LogCategoryAppServer, "ServerView::Paint() failed to send message: {}", strerror(get_last_error()));
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerView::RequestPaintIfNeeded()
{
    if ( m_HideCount > 0) {
        return;
    }
    //    if ( m_pcWindow != nullptr && ((m_pcWindow->GetDesktopMask() & (1 << get_active_desktop())) == 0 ||
    //                              m_pcWindow->HasPendingSizeEvents( this )) ) {
    //      return;
    //    }
    
    if (m_DamageReg != nullptr && m_ActiveDamageReg == nullptr)
    {
        m_ActiveDamageReg = m_DamageReg;
        m_DamageReg = nullptr;
        m_ActiveDamageReg->Optimize();
        Paint( static_cast<PRect>(m_ActiveDamageReg->GetBounds()));
    }
    for (Ptr<PServerView> child : m_ChildrenList) {
        child->RequestPaintIfNeeded();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerView::MarkModified(const PIRect& rect)
{
    if (GetNormalizedBounds().DoIntersect(rect))
    {
        m_HasInvalidRegs = true;
        const PIPoint scrollOffset(m_ScrollOffset);
        
        for (Ptr<PServerView> child : m_ChildrenList) {
            child->MarkModified(rect - child->GetITopLeft() - scrollOffset);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerView::SetDirtyRegFlags()
{
    m_HasInvalidRegs = true;
    
    for (Ptr<PServerView> child : m_ChildrenList) {
        child->SetDirtyRegFlags();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerView::Show(bool doShow)
{
    const bool wasVisible = IsVisible();

    if ( doShow ) {
        m_HideCount--;
    } else {
        m_HideCount++;
    }

    const bool isVisible = IsVisible();
    if (isVisible != wasVisible)
    {

        const Ptr<PServerView> parent = m_Parent.Lock();
        if (parent != nullptr)
        {
            for (Ptr<PServerView> i = parent; i != nullptr; i = i->GetParent())
            {
                if ((i->m_Flags & PViewFlags::Transparent) == 0)
                {
                    i->SetDirtyRegFlags();
                    break;
                }
            }
            const PIRect intFrame = GetIFrame();
            for (Ptr<PServerView> sibling : parent->m_ChildrenList)
            {
                const PIRect siblingIFrame = sibling->GetIFrame();
                if (siblingIFrame.DoIntersect(intFrame)) {
                    sibling->MarkModified(intFrame - siblingIFrame.TopLeft());
                }
            }
        }
        for (auto child = m_ChildrenList.rbegin(); child != m_ChildrenList.rend(); ++child) {
            (*child)->Show(isVisible);
        }
        Invalidate(true);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerView::SetFocusKeyboardMode(PFocusKeyboardMode mode)
{
    if (mode != m_FocusKeyboardMode)
    {
        m_FocusKeyboardMode = mode;

        ApplicationServer* server = static_cast<ApplicationServer*>(GetLooper());
        if (server != nullptr) {
            server->UpdateViewFocusMode(this);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerView::DrawLineTo(const PPoint& toPoint)
{
    DrawLine(m_PenPosition, toPoint);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerView::DrawLine(const PPoint& fromPnt, const PPoint& toPnt)
{
    DrawPolyline(std::vector<PPoint>{ fromPnt, toPnt });
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerView::DrawThinLine(const PPoint& fromPnt, const PPoint& toPnt )
{
    Ptr<const PRegion> region = GetRegion();
    if (region != nullptr)
    {
        const PIPoint screenPos(m_ScreenPos);
        PIPoint fromPntScr(fromPnt + m_ScrollOffset);
        PIPoint toPntScr(toPnt + m_ScrollOffset);

        PIRect boundingBox;
        if (fromPntScr.x > toPntScr.x) {
            std::swap(toPntScr, fromPntScr);
        }
        boundingBox.left  = fromPntScr.x;
        boundingBox.right = toPntScr.x + 1;
        
        if (fromPntScr.y < toPntScr.y) {
            boundingBox.top    = fromPntScr.y;
            boundingBox.bottom = toPntScr.y + 1;
        } else {
            boundingBox.top  = toPntScr.y;
            boundingBox.bottom = fromPntScr.y + 1;
        }

        fromPntScr += screenPos;
        toPntScr   += screenPos;
        
        const PIRect screenFrame = ApplicationServer::GetScreenIFrame();

        if (PRegion::ClipLine(screenFrame, &fromPntScr, &toPntScr))
        {
            for (const PIRect& clip : region->m_Rects)
            {
                if (clip.DoIntersect(boundingBox))
                {
                    m_Bitmap->m_Driver->DrawLine(m_Bitmap, clip + screenPos, fromPntScr, toPntScr, m_FgColor, m_DrawingMode);
                }
            }
        }
    }
    m_PenPosition = toPnt;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerView::DrawRect(const PRect& frame)
{
    DrawLine(PPoint(frame.left, frame.top), PPoint(frame.right - 1.0f, frame.top));
    DrawLine(PPoint(frame.right - 1.0f, frame.top), PPoint(frame.right - 1.0f, frame.bottom - 1.0f));
    DrawLine(PPoint(frame.right - 1.0f, frame.bottom - 1.0f), PPoint(frame.left, frame.bottom - 1.0f));
    DrawLine(PPoint(frame.left, frame.bottom - 1.0f), PPoint(frame.left, frame.top));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerView::FillRect(const PRect& rect, PColor color)
{
    const Ptr<const PRegion> region = GetRegion();
    if (region != nullptr)
    {
        const PIPoint screenPos(m_ScreenPos);
        const PIRect  rectScr(rect + m_ScrollOffset);
        
        m_Bitmap->m_Driver->SetFgColor(color);

        for (const PIRect& clip : region->m_Rects)
        {
            PIRect clippedRect(rectScr & clip);
            if (clippedRect.IsValid()) {
                clippedRect += screenPos;
                m_Bitmap->m_Driver->FillRect(m_Bitmap, clippedRect);
            }
        }
    }        
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerView::FillCircle(const PPoint& position, float radius)
{
    const Ptr<const PRegion> region = GetRegion();
    if (region != nullptr)
    {
        const PIPoint screenPos(m_ScreenPos);
        PIPoint positionScr(position + m_ScrollOffset);
        const int    radiusRounded = int(radius + 0.5f);
        
        positionScr += screenPos;
        
        const PIRect boundingBox(positionScr.x - radiusRounded - 2, positionScr.y - radiusRounded - 2, positionScr.x + radiusRounded + 2, positionScr.y + radiusRounded + 2);
        for (const PIRect& clip : region->m_Rects)
        {
            const PIRect clipRect = clip + screenPos;
            if (!boundingBox.DoIntersect(clipRect)) {
                continue;
            }
            m_Bitmap->m_Driver->FillCircle(m_Bitmap, clipRect, positionScr, int32_t(roundf(radius)), m_FgColor, m_DrawingMode);
        }
    }    
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerView::FillTriangle(const PPoint& pos1, const PPoint& pos2, const PPoint& pos3)
{
    const Ptr<const PRegion> region = GetRegion();
    if (region == nullptr) {
        return;
    }

    const PIPoint screenPos(m_ScreenPos);
    const PPoint screenOffset = PPoint(screenPos) + m_ScrollOffset;
    const PPoint screenPoint1 = pos1 + screenOffset;
    const PPoint screenPoint2 = pos2 + screenOffset;
    const PPoint screenPoint3 = pos3 + screenOffset;
    const PIRect screenFrame = ApplicationServer::GetScreenIFrame();

    const float minX = std::min({ screenPoint1.x, screenPoint2.x, screenPoint3.x });
    const float minY = std::min({ screenPoint1.y, screenPoint2.y, screenPoint3.y });
    const float maxX = std::max({ screenPoint1.x, screenPoint2.x, screenPoint3.x });
    const float maxY = std::max({ screenPoint1.y, screenPoint2.y, screenPoint3.y });
    const PIRect boundingBox(PRect(std::floor(minX), std::floor(minY), std::ceil(maxX) + 1.0f, std::ceil(maxY) + 1.0f));

    if (!screenFrame.DoIntersect(boundingBox)) {
        return;
    }

    std::array<PPoint, 8> clippedPoints =
        {
            screenPoint1,
            screenPoint2,
            screenPoint3
        };

    const size_t clippedPointCount = ClipTrianglePolygon(clippedPoints, 3, screenFrame);

    if (clippedPointCount < 3) {
        return;
    }

    std::array<PIPoint, 8> integerPoints;
    PIRect clippedBoundingBox;
    for (size_t pointIndex = 0; pointIndex < clippedPointCount; ++pointIndex)
    {
        integerPoints[pointIndex] = RoundAndClampTrianglePoint(clippedPoints[pointIndex], screenFrame);
        clippedBoundingBox |= integerPoints[pointIndex];
    }
    ++clippedBoundingBox.right;
    ++clippedBoundingBox.bottom;

    m_Bitmap->m_Driver->SetFgColor(m_FgColor);

    for (const PIRect& clip : region->m_Rects)
    {
        const PIRect clipRect = clip + screenPos;
        if (clipRect.DoIntersect(clippedBoundingBox))
        {
            for (size_t pointIndex = 1; pointIndex < clippedPointCount - 1; ++pointIndex)
            {
                m_Bitmap->m_Driver->FillTriangle(
                    m_Bitmap,
                    clipRect,
                    integerPoints[0],
                    integerPoints[pointIndex],
                    integerPoints[pointIndex + 1],
                    m_DrawingMode);
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerView::FillTriangleFan(std::span<const PPoint> points)
{
    if (points.size() < 3) {
        return;
    }

    const Ptr<const PRegion> region = GetRegion();
    if (region == nullptr) {
        return;
    }

    const PIPoint screenPos(m_ScreenPos);
    const PPoint screenOffset = PPoint(screenPos) + m_ScrollOffset;
    std::vector<PPoint> screenPoints;

    screenPoints.reserve(points.size());
    for (const PPoint& point : points)
    {
        const PPoint screenPoint = point + screenOffset;
        screenPoints.push_back(screenPoint);
    }

    m_Bitmap->m_Driver->SetFgColor(m_FgColor);

    for (const PIRect& clip : region->m_Rects) {
        m_Bitmap->m_Driver->FillTriangleFan(m_Bitmap, clip + screenPos, screenPoints, m_DrawingMode);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerView::FillTriangleStrip(std::span<const PPoint> points)
{
    if (points.size() < 3) {
        return;
    }

    const Ptr<const PRegion> region = GetRegion();
    if (region == nullptr) {
        return;
    }

    const PIPoint screenPos(m_ScreenPos);
    const PPoint screenOffset = PPoint(screenPos) + m_ScrollOffset;
    std::vector<PPoint> screenPoints;

    screenPoints.reserve(points.size());
    for (const PPoint& point : points)
    {
        const PPoint screenPoint = point + screenOffset;
        screenPoints.push_back(screenPoint);
    }

    m_Bitmap->m_Driver->SetFgColor(m_FgColor);

    for (const PIRect& clip : region->m_Rects) {
        m_Bitmap->m_Driver->FillTriangleStrip(m_Bitmap, clip + screenPos, screenPoints, m_DrawingMode);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerView::BeginTriangles(PTriangleMode mode, size_t countHint)
{
    m_TriangleMode = mode;
    m_TrianglePoints.clear();
    m_TrianglePoints.reserve(countHint);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerView::AddTriangle(const PPoint& position)
{
    m_TrianglePoints.push_back(position);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerView::EndTriangles()
{
    switch (m_TriangleMode)
    {
        case PTriangleMode::Fan:
            FillTriangleFan(m_TrianglePoints);
            break;
        case PTriangleMode::Strip:
            FillTriangleStrip(m_TrianglePoints);
            break;
    }
    m_TrianglePoints.clear();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerView::DrawPolyline(std::span<const PPoint> points)
{
    if (points.size() < 2) {
        return;
    }
    if (m_PenWidth > 1.0f)
    {
        if (m_DashPattern.empty()) {
            RenderSolidPolyline(points);
        } else {
            RenderDashedPolyline(points);
        }
    }
    else
    {
        if (m_DashPattern.empty()) {
            for (size_t pointIndex = 1; pointIndex < points.size(); ++pointIndex) {
                DrawThinLine(points[pointIndex - 1], points[pointIndex]);
            }
        } else {
            RenderDashedThinPolyline(points);
        }
    }
    m_PenPosition = points.back();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerView::BeginPolyline()
{
    m_PolylinePoints.clear();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerView::AddPolylinePoint(const PPoint& point)
{
    m_PolylinePoints.push_back(point);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerView::EndPolyline()
{
    if (m_PolylinePoints.size() < 2)
    {
        m_PolylinePoints.clear();
        return;
    }
    DrawPolyline(m_PolylinePoints);
    m_PolylinePoints.clear();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerView::RenderSolidPolyline(std::span<const PPoint> points)
{
    std::vector<PolylineSegData>   segs;
    std::vector<PolylineJointData> joints;

    const float halfWidth = m_PenWidth * 0.5f;

    BuildPolylineGeometry(points, halfWidth, segs, joints);
    if (segs.empty()) {
        return;
    }
    std::vector<PPoint> strip;
    BuildPolylineElement(segs, joints, halfWidth,
                         0, 0.0f, segs.size() - 1, 1.0f,
                         true, true,
                         strip);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerView::RenderDashedPolyline(std::span<const PPoint> points)
{
    const size_t patternSize = m_DashPattern.size();
    if (patternSize == 0) {
        return;
    }
    const float halfWidth = m_PenWidth * 0.5f;

    std::vector<PolylineSegData>   segs;
    std::vector<PolylineJointData> joints;
    BuildPolylineGeometry(points, halfWidth, segs, joints);
    if (segs.empty()) {
        return;
    }

    const float totalLen = segs.back().cumLen + segs.back().segLen;
    const float capExtension = (m_CapStyle == PCapStyle::Flat) ? 0.0f : halfWidth;
    const float renderStart = -capExtension;
    const float renderEnd = totalLen + capExtension;
    float patternLength = 0.0f;
    for (const float elementLength : m_DashPattern) {
        patternLength += elementLength;
    }
    if (patternLength < 1e-6f) {
        return;
    }

    size_t patternIndex = 0;
    float patternPosition = m_DashOffset;
    patternPosition = std::fmod(patternPosition, patternLength);
    if (patternPosition < 0.0f) {
        patternPosition += patternLength;
    }
    while (patternPosition >= m_DashPattern[patternIndex])
    {
        patternPosition -= m_DashPattern[patternIndex];
        patternIndex = (patternIndex + 1) % patternSize;
    }
    float dashPos = patternPosition / m_DashPattern[patternIndex];

    // Clip the solid geometry to each dash/space interval so adjacent colors
    // partition the same footprint as the undashed polyline.
    float arcPos = renderStart;

    while (arcPos < renderEnd - 1e-6f)
    {
        const float elementLen  = m_DashPattern[patternIndex] * m_PenWidth;
        const float remainInEl  = elementLen * (1.0f - dashPos);
        const float step        = std::min(remainInEl, renderEnd - arcPos);
        const bool  isSolid     = (patternIndex % 2) == 0;
        const float arcStart    = arcPos;
        const float arcEnd      = arcPos + step;

        if (isSolid)
        {
            RenderClippedPolylineElement(segs, joints, halfWidth,
                                         arcStart, arcEnd, totalLen);
        }
        else if (m_DrawingMode == PDrawingMode::Copy)
        {
            const PColor savedFgColor = m_FgColor;
            m_FgColor = m_BgColor;
            RenderClippedPolylineElement(segs, joints, halfWidth,
                                         arcStart, arcEnd, totalLen);
            m_FgColor = savedFgColor;
        }

        arcPos  += step;
        dashPos += step / elementLen;
        if (dashPos >= 1.0f - 1e-6f)
        {
            dashPos      = 0.0f;
            patternIndex = (patternIndex + 1) % patternSize;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerView::RenderDashedThinPolyline(std::span<const PPoint> points)
{
    const size_t patternSize = m_DashPattern.size();
    if (patternSize == 0) {
        return;
    }

    float patternLength = 0.0f;
    for (const float elementLength : m_DashPattern) {
        patternLength += elementLength;
    }
    if (patternLength < 1e-6f) {
        return;
    }

    size_t patternIndex = 0;
    float patternPosition = std::fmod(m_DashOffset, patternLength);
    if (patternPosition < 0.0f) {
        patternPosition += patternLength;
    }
    while (patternPosition >= m_DashPattern[patternIndex])
    {
        patternPosition -= m_DashPattern[patternIndex];
        patternIndex = (patternIndex + 1) % patternSize;
    }
    float dashPosition = patternPosition / m_DashPattern[patternIndex];
    const float patternScale = std::max(m_PenWidth, 1.0f);

    for (size_t pointIndex = 1; pointIndex < points.size(); ++pointIndex)
    {
        const PPoint segmentStart = points[pointIndex - 1];
        const PPoint segmentDelta = points[pointIndex] - segmentStart;
        const float segmentLength = segmentDelta.Length();
        if (segmentLength < 1e-6f) {
            continue;
        }

        const PPoint segmentDirection = segmentDelta / segmentLength;
        float segmentPosition = 0.0f;
        while (segmentPosition < segmentLength - 1e-6f)
        {
            const float elementLength = m_DashPattern[patternIndex] * patternScale;
            const float remainingLength = elementLength * (1.0f - dashPosition);
            const float stepLength = std::min(remainingLength, segmentLength - segmentPosition);
            const PPoint dashStart = segmentStart + segmentDirection * segmentPosition;
            const PPoint dashEnd = dashStart + segmentDirection * stepLength;
            const bool isSolid = (patternIndex % 2) == 0;

            if (isSolid) {
                DrawThinLine(dashStart, dashEnd);
            } else if (m_DrawingMode == PDrawingMode::Copy) {
                const PColor savedFgColor = m_FgColor;
                m_FgColor = m_BgColor;
                DrawThinLine(dashStart, dashEnd);
                m_FgColor = savedFgColor;
            }

            segmentPosition += stepLength;
            dashPosition += stepLength / elementLength;
            if (dashPosition >= 1.0f - 1e-6f)
            {
                dashPosition = 0.0f;
                patternIndex = (patternIndex + 1) % patternSize;
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerView::BuildPolylineGeometry(std::span<const PPoint> points, float halfWidth,
                                        std::vector<PolylineSegData>&   outSegs,
                                        std::vector<PolylineJointData>& outJoints)
{
    outSegs.clear();
    outJoints.clear();

    // First pass: collect non-degenerate segments.
    struct RawSeg
    {
        PPoint srcStart;
        PPoint dir;
        PPoint norm;
        float  length;
    };
    std::vector<RawSeg> rawSegs;
    rawSegs.reserve(points.size());

    for (size_t i = 0; i + 1 < points.size(); ++i)
    {
        const PPoint delta = points[i + 1] - points[i];
        const float  len   = delta.Length();
        if (len < 1e-6f) {
            continue;
        }
        const PPoint dir = delta / len;
        rawSegs.push_back({ points[i], dir, PPoint(-dir.y, dir.x), len });
    }

    if (rawSegs.empty()) {
        return;
    }

    const size_t segCount = rawSegs.size();
    outSegs.resize(segCount);
    outJoints.resize(segCount); // outJoints[i] = joint between seg i-1 and seg i

    // Second pass: compute joint data at each interior vertex.
    for (size_t i = 1; i < segCount; ++i)
    {
        const PPoint& prevDir  = rawSegs[i - 1].dir;
        const PPoint& prevNorm = rawSegs[i - 1].norm;
        const PPoint& dir      = rawSegs[i].dir;
        const PPoint& norm     = rawSegs[i].norm;
        const PPoint& vertex   = rawSegs[i].srcStart;

        PolylineJointData& joint = outJoints[i];
        joint.vertex  = vertex;
        joint.prevDir = prevDir;
        joint.dir     = dir;

        const float crossZ  = prevDir.x * dir.y - prevDir.y * dir.x;
        const float dotProd = prevDir.x * dir.x + prevDir.y * dir.y;
        joint.crossZ = crossZ;

        if (std::abs(crossZ) < 1e-4f)
        {
            if (dotProd >= 0.0f)
            {
                joint.isCollinear = true;
                joint.innerPoint  = vertex;
                joint.outerPrev   = vertex;
                joint.outerNext   = vertex;
            }
            else
            {
                joint.isHairpin  = true;
                joint.innerPoint = vertex;
                joint.sign = (crossZ >= 0.0f) ? -1.0f : 1.0f;
                joint.outerPrev = vertex + prevNorm * joint.sign * halfWidth;
                joint.outerNext = vertex + norm     * joint.sign * halfWidth;
            }
        }
        else
        {
            joint.sign = (crossZ < 0.0f) ? 1.0f : -1.0f;

            const PPoint outerPrev     = vertex + prevNorm * joint.sign * halfWidth;
            const PPoint outerNext     = vertex + norm     * joint.sign * halfWidth;
            const PPoint innerEdgePrev = vertex - prevNorm * joint.sign * halfWidth;
            const PPoint innerEdgeNext = vertex - norm     * joint.sign * halfWidth;
            const PPoint innerDelta    = innerEdgeNext - innerEdgePrev;
            const float  t             = (innerDelta.x * dir.y - innerDelta.y * dir.x) / crossZ;
            const PPoint candidateInner = innerEdgePrev + prevDir * t;
            const PPoint nextInnerOffset = candidateInner - innerEdgeNext;
            const float nextT = nextInnerOffset.x * dir.x + nextInnerOffset.y * dir.y;
            const bool extendsBeforePreviousSegment = t < -rawSegs[i - 1].length;
            const bool extendsPastNextSegment = nextT > rawSegs[i].length;

            if (extendsBeforePreviousSegment || extendsPastNextSegment)
            {
                joint.isHairpin  = true;
                joint.innerPoint = vertex;
                joint.outerPrev  = outerPrev;
                joint.outerNext  = outerNext;
            }
            else
            {
                if ((candidateInner - vertex).Length() > halfWidth * 10.0f + 100.0f)
                {
                    joint.isHairpin  = true;
                    joint.innerPoint = vertex;
                }
                else
                {
                    joint.innerPoint = candidateInner;
                }
                joint.outerPrev = outerPrev;
                joint.outerNext = outerNext;
            }
        }
    }

    // Third pass: assign segment corner points using the joint data.
    float cumLen = 0.0f;
    for (size_t i = 0; i < segCount; ++i)
    {
        PolylineSegData& seg = outSegs[i];
        seg.dir      = rawSegs[i].dir;
        seg.norm     = rawSegs[i].norm;
        seg.segLen   = rawSegs[i].length;
        seg.cumLen   = cumLen;
        seg.srcStart = rawSegs[i].srcStart;
        cumLen      += seg.segLen;

        // Start corners.
        if (i == 0)
        {
            seg.startL = seg.srcStart - seg.norm * halfWidth;
            seg.startR = seg.srcStart + seg.norm * halfWidth;
        }
        else
        {
            const PolylineJointData& joint = outJoints[i];
            if (joint.isCollinear || joint.isHairpin)
            {
                seg.startL = joint.vertex - seg.norm * halfWidth;
                seg.startR = joint.vertex + seg.norm * halfWidth;
            }
            else if (joint.sign > 0.0f)
            {
                seg.startL = joint.innerPoint;
                seg.startR = joint.outerNext;
            }
            else
            {
                seg.startL = joint.outerNext;
                seg.startR = joint.innerPoint;
            }
        }

        // End corners.
        if (i + 1 == segCount)
        {
            const PPoint srcEnd = seg.srcStart + seg.dir * seg.segLen;
            seg.endL = srcEnd - seg.norm * halfWidth;
            seg.endR = srcEnd + seg.norm * halfWidth;
        }
        else
        {
            const PolylineJointData& nextJoint = outJoints[i + 1];
            if (nextJoint.isCollinear || nextJoint.isHairpin)
            {
                seg.endL = nextJoint.vertex - seg.norm * halfWidth;
                seg.endR = nextJoint.vertex + seg.norm * halfWidth;
            }
            else if (nextJoint.sign > 0.0f)
            {
                seg.endL = nextJoint.innerPoint;
                seg.endR = nextJoint.outerPrev;
            }
            else
            {
                seg.endL = nextJoint.outerPrev;
                seg.endR = nextJoint.innerPoint;
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerView::EmitJointToStrip(std::vector<PPoint>& strip,
                                   std::vector<PPoint>& additionalTrianglePoints,
                                   const PolylineJointData& joint,
                                   float halfWidth)
{
    // sign > 0 (right turn, outer on R/+norm side):
    //   On entry strip ends with (innerPoint=L, outerPrev=R).
    //   The cap geometry falls naturally from the strip: the odd slot (n+1) holds
    //   outerPrev, so the bevel/miter/arc triangle is geometrically correct.
    //   On exit strip ends with (innerPoint=L, outerNext=R).
    //
    // sign < 0 (left turn, outer on L/−norm side):
    //   On entry strip ends with (outerPrev=L, innerPoint=R).
    //   The odd slot holds innerPoint, not outerPrev, so building the cap from the
    //   strip would draw the wrong triangle. Render cap geometry via separate
    //   FillTriangle / fan calls instead, then bridge the strip.
    //   Strip bridge: +vertex, +innerPoint, +outerNext, +innerPoint
    //     → triangle n (even): incoming wedge (outerPrev, innerPoint, vertex)
    //     → triangle n+1 (odd): degenerate
    //     → triangle n+2 (even): outgoing wedge (vertex, innerPoint, outerNext)
    //     → triangle n+3 (odd): degenerate
    //   On exit strip ends with (outerNext=L, innerPoint=R).

    const PPoint& vertex     = joint.vertex;
    const PPoint& innerPoint = joint.innerPoint;
    const PPoint& outerPrev  = joint.outerPrev;
    const PPoint& outerNext  = joint.outerNext;

    if (m_JointStyle == PJointStyle::Round)
    {
        AppendPolylineRoundJointTriangles(additionalTrianglePoints,
                                           vertex, innerPoint, outerPrev, outerNext,
                                           joint.crossZ, halfWidth);

        // Restart the stem strip at the next segment without drawing the joint
        // body. The complete rounded joint was submitted as one fan above.
        strip.push_back(strip.back());
        if (joint.sign > 0.0f)
        {
            strip.push_back(innerPoint);
            strip.push_back(innerPoint);
            strip.push_back(outerNext);
        }
        else
        {
            strip.push_back(outerNext);
            strip.push_back(outerNext);
            strip.push_back(innerPoint);
        }
        return;
    }

    if (joint.sign > 0.0f)
    {
        // Outer is on the R (+norm) side. Strip ends (innerPoint=L, outerPrev=R).
        switch (m_JointStyle)
        {
            case PJointStyle::Bevel:
                strip.push_back(vertex);
                strip.push_back(outerNext);
                strip.push_back(innerPoint);
                strip.push_back(outerNext);
                break;

            case PJointStyle::Miter:
            {
                const float denom    = joint.prevDir.x * joint.dir.y - joint.prevDir.y * joint.dir.x;
                bool        useBevel = (std::abs(denom) < 1e-6f);
                PPoint      miterPoint;

                if (!useBevel)
                {
                    const PPoint delta = outerNext - outerPrev;
                    const float  mt    = (delta.x * joint.dir.y - delta.y * joint.dir.x) / denom;
                    miterPoint = outerPrev + joint.prevDir * mt;
                    useBevel   = ((miterPoint - vertex).Length() > m_MiterLimit * m_PenWidth);
                }

                if (useBevel)
                {
                    strip.push_back(vertex);
                    strip.push_back(outerNext);
                    strip.push_back(innerPoint);
                    strip.push_back(outerNext);
                }
                else
                {
                    strip.push_back(vertex);
                    strip.push_back(miterPoint);
                    strip.push_back(vertex);
                    strip.push_back(outerNext);
                    strip.push_back(innerPoint);
                    strip.push_back(outerNext);
                }
                break;
            }
            case PJointStyle::Round:
                break;
        }
    }
    else
    {
        // Outer is on the L (−norm) side. Strip ends (outerPrev=L, innerPoint=R).
        // Render the cap as separate triangle(s) so innerPoint is never used as a
        // cap vertex (avoids spikes when innerPoint is far from vertex).
        switch (m_JointStyle)
        {
            case PJointStyle::Bevel:
            {
                AppendPolylineBevelJointTriangle(additionalTrianglePoints,
                                                  vertex, outerPrev, outerNext);
                break;
            }
            case PJointStyle::Miter:
            {
                const float denom    = joint.prevDir.x * joint.dir.y - joint.prevDir.y * joint.dir.x;
                bool        useBevel = (std::abs(denom) < 1e-6f);
                PPoint      miterPoint;

                if (!useBevel)
                {
                    const PPoint delta = outerNext - outerPrev;
                    const float  mt    = (delta.x * joint.dir.y - delta.y * joint.dir.x) / denom;
                    miterPoint = outerPrev + joint.prevDir * mt;
                    useBevel   = ((miterPoint - vertex).Length() > m_MiterLimit * m_PenWidth);
                }

                if (useBevel)
                {
                    AppendPolylineBevelJointTriangle(additionalTrianglePoints,
                                                      vertex, outerPrev, outerNext);
                }
                else
                {
                    additionalTrianglePoints.push_back(vertex);
                    additionalTrianglePoints.push_back(outerPrev);
                    additionalTrianglePoints.push_back(miterPoint);
                    additionalTrianglePoints.push_back(vertex);
                    additionalTrianglePoints.push_back(miterPoint);
                    additionalTrianglePoints.push_back(outerNext);
                }
                break;
            }
            case PJointStyle::Round:
            {
                break;
            }
        }
        // Bridge strip to (outerNext=L, innerPoint=R).
        strip.push_back(vertex);
        strip.push_back(innerPoint);
        strip.push_back(outerNext);
        strip.push_back(innerPoint);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerView::FillClippedPolylineTriangle(const PPoint& point1, float point1Arc,
                                              const PPoint& point2, float point2Arc,
                                              const PPoint& point3, float point3Arc,
                                              float arcStart, float arcEnd)
{
    std::array<PolylineClipVertex, 8> clippedPoints =
        {{
            { point1, point1Arc },
            { point2, point2Arc },
            { point3, point3Arc }
        }};
    std::array<PolylineClipVertex, 8> outputPoints;
    size_t clippedPointCount = 3;

    clippedPointCount = ClipPolylineTriangleBoundary(
        clippedPoints, clippedPointCount, outputPoints, arcStart, true);
    clippedPoints = outputPoints;
    clippedPointCount = ClipPolylineTriangleBoundary(
        clippedPoints, clippedPointCount, outputPoints, arcEnd, false);
    clippedPoints = outputPoints;

    if (clippedPointCount < 3) {
        return;
    }

    std::array<PPoint, 8> polygonPoints;
    for (size_t pointIndex = 0; pointIndex < clippedPointCount; ++pointIndex) {
        polygonPoints[pointIndex] = clippedPoints[pointIndex].Point;
    }
    FillTriangleFan(std::span<const PPoint>(polygonPoints.data(), clippedPointCount));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerView::FillClippedPolylinePolygon(std::span<const PPoint> points,
                                             std::span<const float> arcPositions,
                                             float arcStart,
                                             float arcEnd)
{
    if (points.size() != arcPositions.size() || points.size() < 3 || points.size() > 6) {
        return;
    }

    std::array<PolylineClipVertex, 8> clippedPoints;
    std::array<PolylineClipVertex, 8> outputPoints;
    size_t clippedPointCount = points.size();

    for (size_t pointIndex = 0; pointIndex < points.size(); ++pointIndex)
    {
        clippedPoints[pointIndex] = { points[pointIndex], arcPositions[pointIndex] };
    }

    clippedPointCount = ClipPolylineTriangleBoundary(
        clippedPoints, clippedPointCount, outputPoints, arcStart, true);
    clippedPoints = outputPoints;
    clippedPointCount = ClipPolylineTriangleBoundary(
        clippedPoints, clippedPointCount, outputPoints, arcEnd, false);
    clippedPoints = outputPoints;

    if (clippedPointCount < 3) {
        return;
    }

    std::array<PPoint, 8> polygonPoints;
    for (size_t pointIndex = 0; pointIndex < clippedPointCount; ++pointIndex) {
        polygonPoints[pointIndex] = clippedPoints[pointIndex].Point;
    }
    FillTriangleFan(std::span<const PPoint>(polygonPoints.data(), clippedPointCount));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerView::RenderClippedPolylineJoint(const std::vector<PolylineSegData>& segs,
                                             const PolylineJointData& joint,
                                             size_t jointIndex,
                                             float halfWidth,
                                             float arcStart,
                                             float arcEnd)
{
    if (joint.isCollinear) {
        return;
    }

    const PolylineSegData& previousSegment = segs[jointIndex - 1];
    const PolylineSegData& nextSegment = segs[jointIndex];
    const float vertexArc = nextSegment.cumLen;
    const auto previousArc = [&previousSegment](const PPoint& point)
        {
            const PPoint offset = point - previousSegment.srcStart;
            return previousSegment.cumLen + offset.x * previousSegment.dir.x + offset.y * previousSegment.dir.y;
        };
    const auto nextArc = [&nextSegment](const PPoint& point)
        {
            const PPoint offset = point - nextSegment.srcStart;
            return nextSegment.cumLen + offset.x * nextSegment.dir.x + offset.y * nextSegment.dir.y;
        };
    const float outerPreviousArc = previousArc(joint.outerPrev);
    const float outerNextArc = nextArc(joint.outerNext);

    FillClippedPolylineTriangle(joint.innerPoint, previousArc(joint.innerPoint),
                                joint.outerPrev, outerPreviousArc,
                                joint.vertex, vertexArc,
                                arcStart, arcEnd);
    FillClippedPolylineTriangle(joint.vertex, vertexArc,
                                joint.outerNext, outerNextArc,
                                joint.innerPoint, nextArc(joint.innerPoint),
                                arcStart, arcEnd);

    if (m_JointStyle == PJointStyle::Bevel || joint.isHairpin)
    {
        FillClippedPolylineTriangle(joint.vertex, vertexArc,
                                    joint.outerPrev, outerPreviousArc,
                                    joint.outerNext, outerNextArc,
                                    arcStart, arcEnd);
        return;
    }

    if (m_JointStyle == PJointStyle::Miter)
    {
        const float denominator = joint.prevDir.x * joint.dir.y - joint.prevDir.y * joint.dir.x;
        bool useBevel = (std::abs(denominator) < 1e-6f);
        PPoint miterPoint;

        if (!useBevel)
        {
            const PPoint delta = joint.outerNext - joint.outerPrev;
            const float multiplier = (delta.x * joint.dir.y - delta.y * joint.dir.x) / denominator;
            miterPoint = joint.outerPrev + joint.prevDir * multiplier;
            useBevel = ((miterPoint - joint.vertex).Length() > m_MiterLimit * m_PenWidth);
        }

        if (useBevel)
        {
            FillClippedPolylineTriangle(joint.vertex, vertexArc,
                                        joint.outerPrev, outerPreviousArc,
                                        joint.outerNext, outerNextArc,
                                        arcStart, arcEnd);
        }
        else
        {
            const float miterArc = (outerPreviousArc + outerNextArc) * 0.5f;
            FillClippedPolylineTriangle(joint.vertex, vertexArc,
                                        joint.outerPrev, outerPreviousArc,
                                        miterPoint, miterArc,
                                        arcStart, arcEnd);
            FillClippedPolylineTriangle(joint.vertex, vertexArc,
                                        miterPoint, miterArc,
                                        joint.outerNext, outerNextArc,
                                        arcStart, arcEnd);
        }
        return;
    }

    static constexpr int ARC_STEPS = 8;
    const float startAngle = std::atan2f(joint.outerPrev.y - joint.vertex.y, joint.outerPrev.x - joint.vertex.x);
    const float endAngle = std::atan2f(joint.outerNext.y - joint.vertex.y, joint.outerNext.x - joint.vertex.x);
    float angleSpan = endAngle - startAngle;

    if (joint.crossZ > 0.0f)
    {
        if (angleSpan < 0.0f) {
            angleSpan += 2.0f * float(M_PI);
        }
    }
    else if (angleSpan > 0.0f)
    {
        angleSpan -= 2.0f * float(M_PI);
    }

    PPoint previousPoint = joint.outerPrev;
    float previousPointArc = outerPreviousArc;
    for (int step = 1; step <= ARC_STEPS; ++step)
    {
        const PPoint nextPoint = (step == ARC_STEPS)
            ? joint.outerNext
            : joint.vertex + PPoint(std::cosf(startAngle + angleSpan * float(step) / float(ARC_STEPS)),
                                    std::sinf(startAngle + angleSpan * float(step) / float(ARC_STEPS))) * halfWidth;
        const float nextPointArc = (step == ARC_STEPS)
            ? outerNextArc
            : outerPreviousArc + (outerNextArc - outerPreviousArc) * float(step) / float(ARC_STEPS);
        FillClippedPolylineTriangle(joint.vertex, vertexArc,
                                    previousPoint, previousPointArc,
                                    nextPoint, nextPointArc,
                                    arcStart, arcEnd);
        previousPoint = nextPoint;
        previousPointArc = nextPointArc;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerView::RenderClippedPolylineCap(const PPoint& endpoint,
                                           const PPoint& lineDirection,
                                           float halfWidth,
                                           bool isStart,
                                           float endpointArc,
                                           float arcStart,
                                           float arcEnd)
{
    if (m_CapStyle == PCapStyle::Flat) {
        return;
    }

    const PPoint normal(-lineDirection.y, lineDirection.x);
    const auto pointArc = [&endpoint, &lineDirection, endpointArc](const PPoint& point)
        {
            const PPoint offset = point - endpoint;
            return endpointArc + offset.x * lineDirection.x + offset.y * lineDirection.y;
        };
    const PPoint endLeft = endpoint - normal * halfWidth;
    const PPoint endRight = endpoint + normal * halfWidth;

    if (m_CapStyle == PCapStyle::Square)
    {
        const PPoint extension = endpoint + lineDirection * (isStart ? -halfWidth : halfWidth);
        const PPoint extensionLeft = extension - normal * halfWidth;
        const PPoint extensionRight = extension + normal * halfWidth;
        const std::array<PPoint, 4> capPoints = {
            endLeft, endRight, extensionRight, extensionLeft
        };
        const std::array<float, 4> capArcs = {
            pointArc(endLeft), pointArc(endRight),
            pointArc(extensionRight), pointArc(extensionLeft)
        };
        FillClippedPolylinePolygon(capPoints, capArcs, arcStart, arcEnd);
        return;
    }

    static constexpr int ARC_STEPS = 8;
    const float startAngle = isStart
        ? std::atan2f(-normal.y, -normal.x)
        : std::atan2f(normal.y, normal.x);
    PPoint previousPoint = isStart ? endLeft : endRight;
    float previousPointArc = pointArc(previousPoint);
    for (int step = 1; step <= ARC_STEPS; ++step)
    {
        const PPoint nextPoint = (step == ARC_STEPS)
            ? (isStart ? endRight : endLeft)
            : endpoint + PPoint(std::cosf(startAngle - float(step) * float(M_PI) / float(ARC_STEPS)),
                                std::sinf(startAngle - float(step) * float(M_PI) / float(ARC_STEPS))) * halfWidth;
        const float nextPointArc = pointArc(nextPoint);
        FillClippedPolylineTriangle(endpoint, endpointArc,
                                    previousPoint, previousPointArc,
                                    nextPoint, nextPointArc,
                                    arcStart, arcEnd);
        previousPoint = nextPoint;
        previousPointArc = nextPointArc;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerView::RenderClippedPolylineElement(const std::vector<PolylineSegData>& segs,
                                               const std::vector<PolylineJointData>& joints,
                                               float halfWidth,
                                               float arcStart,
                                               float arcEnd,
                                               float totalLength)
{
    for (const PolylineSegData& segment : segs)
    {
        const auto pointArc = [&segment](const PPoint& point)
            {
                const PPoint offset = point - segment.srcStart;
                return segment.cumLen + offset.x * segment.dir.x + offset.y * segment.dir.y;
            };
        const std::array<PPoint, 4> segmentPoints = {
            segment.startL, segment.startR, segment.endR, segment.endL
        };
        const std::array<float, 4> segmentArcs = {
            pointArc(segment.startL), pointArc(segment.startR),
            pointArc(segment.endR), pointArc(segment.endL)
        };
        FillClippedPolylinePolygon(segmentPoints, segmentArcs, arcStart, arcEnd);
    }

    for (size_t jointIndex = 1; jointIndex < joints.size(); ++jointIndex) {
        RenderClippedPolylineJoint(segs, joints[jointIndex], jointIndex,
                                   halfWidth, arcStart, arcEnd);
    }

    const PolylineSegData& firstSegment = segs.front();
    RenderClippedPolylineCap(firstSegment.srcStart, firstSegment.dir,
                             halfWidth, true, 0.0f, arcStart, arcEnd);

    const PolylineSegData& finalSegment = segs.back();
    const PPoint finalPoint = finalSegment.srcStart + finalSegment.dir * finalSegment.segLen;
    RenderClippedPolylineCap(finalPoint, finalSegment.dir,
                             halfWidth, false, totalLength, arcStart, arcEnd);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerView::BuildPolylineElement(const std::vector<PolylineSegData>&   segs,
                                       const std::vector<PolylineJointData>& joints,
                                       float halfWidth,
                                       size_t firstSeg, float firstT,
                                       size_t lastSeg,  float lastT,
                                       bool addStartCap, bool addEndCap,
                                       std::vector<PPoint>& strip)
{
    strip.clear();
    std::vector<PPoint> additionalTrianglePoints;

    if (firstSeg > lastSeg) {
        return;
    }
    if (firstSeg == lastSeg && firstT >= lastT - 1e-6f) {
        return;
    }

    const PolylineSegData& startSeg = segs[firstSeg];
    const PolylineSegData& endSeg   = segs[lastSeg];

    // When the element starts exactly at a joint vertex (and the joint is not collinear),
    // this element owns that joint's geometry. The strip begins from the previous segment's
    // end corners (slanted toward the joint) so that EmitJointToStrip receives the correct
    // entry geometry, and the adjacent element ending at this joint tiles perfectly.
    const bool startsAtJoint = (firstT < 1e-6f)
                            && (firstSeg > 0)
                            && !joints[firstSeg].isCollinear;

    PPoint startPt = startSeg.srcStart + startSeg.dir * (firstT * startSeg.segLen);

    if (startsAtJoint)
    {
        const PolylineJointData& joint   = joints[firstSeg];
        const PolylineSegData&   prevSeg = segs[firstSeg - 1];

        // Start from the previous segment's end corners so that EmitJointToStrip
        // receives the correct entry geometry for this joint.
        strip.push_back(prevSeg.endL);
        strip.push_back(prevSeg.endR);

        if (joint.isHairpin)
        {
            switch (m_JointStyle)
            {
                case PJointStyle::Bevel:
                case PJointStyle::Miter:
                    AppendPolylineBevelJointTriangle(additionalTrianglePoints,
                                                      joint.vertex, joint.outerPrev, joint.outerNext);
                    break;
                case PJointStyle::Round:
                    AppendPolylineRoundJointTriangles(additionalTrianglePoints,
                                                       joint.vertex, joint.vertex,
                                                       joint.outerPrev, joint.outerNext,
                                                       joint.crossZ, halfWidth);
                    break;
            }
            strip.push_back(strip.back());
            strip.push_back(segs[firstSeg].startL);
            strip.push_back(segs[firstSeg].startR);
        }
        else
        {
            EmitJointToStrip(strip, additionalTrianglePoints, joint, halfWidth);
        }
    }
    else
    {
        const PPoint startL = startPt - startSeg.norm * halfWidth;
        const PPoint startR = startPt + startSeg.norm * halfWidth;

        if (addStartCap && m_CapStyle == PCapStyle::Square)
        {
            const PPoint ext = startPt - startSeg.dir * halfWidth;
            strip.push_back(ext - startSeg.norm * halfWidth);
            strip.push_back(ext + startSeg.norm * halfWidth);
        }

        strip.push_back(startL);
        strip.push_back(startR);
    }

    for (size_t i = firstSeg; i <= lastSeg; ++i)
    {
        const float endT_i = (i == lastSeg) ? lastT : 1.0f;

        if (i > firstSeg)
        {
            const PolylineJointData& joint = joints[i];
            if (joint.isHairpin)
            {
                // Hairpin: render outer cap separately, then bridge to next segment
                // using degenerate triangles.
                switch (m_JointStyle)
                {
                    case PJointStyle::Bevel:
                    case PJointStyle::Miter:
                        AppendPolylineBevelJointTriangle(additionalTrianglePoints,
                                                          joint.vertex, joint.outerPrev, joint.outerNext);
                        break;
                    case PJointStyle::Round:
                        AppendPolylineRoundJointTriangles(additionalTrianglePoints,
                                                           joint.vertex, joint.vertex,
                                                           joint.outerPrev, joint.outerNext,
                                                           joint.crossZ, halfWidth);
                        break;
                }
                // Degenerate bridge: repeat last point then add next segment's start corners.
                strip.push_back(strip.back());
                strip.push_back(segs[i].startL);
                strip.push_back(segs[i].startR);
            }
            else if (!joint.isCollinear)
            {
                EmitJointToStrip(strip, additionalTrianglePoints, joint, halfWidth);
            }
            // Collinear: prev endL/endR == this seg's startL/startR, just continue.
        }

        PPoint endL;
        PPoint endR;
        if (endT_i >= 1.0f - 1e-6f)
        {
            endL = segs[i].endL;
            endR = segs[i].endR;
        }
        else
        {
            const PPoint endPt = segs[i].srcStart + segs[i].dir * (endT_i * segs[i].segLen);
            endL = endPt - segs[i].norm * halfWidth;
            endR = endPt + segs[i].norm * halfWidth;
        }
        strip.push_back(endL);
        strip.push_back(endR);
    }

    if (addEndCap && m_CapStyle == PCapStyle::Square)
    {
        const PPoint endPt = endSeg.srcStart + endSeg.dir * (lastT * endSeg.segLen);
        const PPoint ext   = endPt + endSeg.dir * halfWidth;
        strip.push_back(ext - endSeg.norm * halfWidth);
        strip.push_back(ext + endSeg.norm * halfWidth);
    }

    if (addStartCap && m_CapStyle == PCapStyle::Round) {
        AppendPolylineCapTriangles(additionalTrianglePoints,
                                    startPt, startSeg.dir, halfWidth, true);
    }
    if (addEndCap && m_CapStyle == PCapStyle::Round)
    {
        const PPoint endPt = endSeg.srcStart + endSeg.dir * (lastT * endSeg.segLen);
        AppendPolylineCapTriangles(additionalTrianglePoints,
                                    endPt, endSeg.dir, halfWidth, false);
    }

    for (size_t pointIndex = 0;
         pointIndex + 2 < additionalTrianglePoints.size();
         pointIndex += 3)
    {
        AppendDisconnectedTriangleToStrip(strip,
                                          additionalTrianglePoints[pointIndex],
                                          additionalTrianglePoints[pointIndex + 1],
                                          additionalTrianglePoints[pointIndex + 2]);
    }

    FillTriangleStrip(strip);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerView::AppendDisconnectedTriangleToStrip(std::vector<PPoint>& strip,
                                                    const PPoint& point1,
                                                    const PPoint& point2,
                                                    const PPoint& point3)
{
    if (strip.empty())
    {
        strip.push_back(point1);
        strip.push_back(point2);
        strip.push_back(point3);
        return;
    }

    strip.push_back(strip.back());
    strip.push_back(point1);
    strip.push_back(point1);
    strip.push_back(point2);
    strip.push_back(point3);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerView::AppendPolylineCapTriangles(std::vector<PPoint>& trianglePoints,
                                             const PPoint& endpoint,
                                             const PPoint& lineDir,
                                             float halfWidth,
                                             bool isStart)
{
    const PPoint norm(-lineDir.y, lineDir.x);

    if (m_CapStyle == PCapStyle::Square)
    {
        const PPoint ext  = endpoint + lineDir * (isStart ? -halfWidth : halfWidth);
        const PPoint endL = endpoint - norm * halfWidth;
        const PPoint endR = endpoint + norm * halfWidth;
        const PPoint extL = ext - norm * halfWidth;
        const PPoint extR = ext + norm * halfWidth;
        trianglePoints.push_back(endL);
        trianglePoints.push_back(endR);
        trianglePoints.push_back(extL);
        trianglePoints.push_back(endR);
        trianglePoints.push_back(extR);
        trianglePoints.push_back(extL);
    }
    else // Round
    {
        // Half-circle fan. For start cap sweep CW from -norm through -lineDir to +norm.
        // For end cap sweep CW from +norm through +lineDir to -norm.
        const float startA = isStart
            ? std::atan2f(-norm.y, -norm.x)
            : std::atan2f(norm.y, norm.x);

        // Use exact corner points for the first and last arc vertices to avoid
        // sub-pixel seam gaps caused by atan2→cos/sin floating-point round-trip.
        const PPoint firstArc = isStart ? (endpoint - norm * halfWidth) : (endpoint + norm * halfWidth);
        const PPoint lastArc  = isStart ? (endpoint + norm * halfWidth) : (endpoint - norm * halfWidth);
        static constexpr int ARC_STEPS = 8;
        PPoint previousPoint = firstArc;
        for (int step = 1; step < ARC_STEPS; ++step)
        {
            const float angle = startA - float(step) * float(M_PI) / float(ARC_STEPS);
            const PPoint nextPoint = endpoint + PPoint(std::cosf(angle), std::sinf(angle)) * halfWidth;
            trianglePoints.push_back(endpoint);
            trianglePoints.push_back(previousPoint);
            trianglePoints.push_back(nextPoint);
            previousPoint = nextPoint;
        }
        trianglePoints.push_back(endpoint);
        trianglePoints.push_back(previousPoint);
        trianglePoints.push_back(lastArc);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerView::AppendPolylineBevelJointTriangle(std::vector<PPoint>& trianglePoints,
                                                   const PPoint& vertex,
                                                   const PPoint& outerPrev,
                                                   const PPoint& outerNext)
{
    trianglePoints.push_back(vertex);
    trianglePoints.push_back(outerPrev);
    trianglePoints.push_back(outerNext);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerView::RenderPolylineMiterJoint(const PPoint& vertex,
                                           const PPoint& outerPrev,
                                           const PPoint& outerNext,
                                           const PPoint& dirPrev,
                                           const PPoint& dirNext)
{
    const float denom = dirPrev.x * dirNext.y - dirPrev.y * dirNext.x;
    if (std::abs(denom) < 1e-6f)
    {
        FillTriangle(vertex, outerPrev, outerNext);
        return;
    }

    const PPoint delta = outerNext - outerPrev;
    const float  t     = (delta.x * dirNext.y - delta.y * dirNext.x) / denom;
    const PPoint miterPoint = outerPrev + dirPrev * t;

    if ((miterPoint - vertex).Length() > m_MiterLimit * m_PenWidth)
    {
        FillTriangle(vertex, outerPrev, outerNext);
        return;
    }

    FillTriangle(vertex, outerPrev, miterPoint);
    FillTriangle(vertex, miterPoint, outerNext);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerView::AppendPolylineRoundJointTriangles(std::vector<PPoint>& trianglePoints,
                                                    const PPoint& vertex,
                                                    const PPoint& innerPoint,
                                                    const PPoint& outerPrev,
                                                    const PPoint& outerNext,
                                                    float crossZ,
                                                    float halfWidth)
{
    float startA = std::atan2f(outerPrev.y - vertex.y, outerPrev.x - vertex.x);
    const float endA = std::atan2f(outerNext.y - vertex.y, outerNext.x - vertex.x);
    float spanA = endA - startA;

    if (crossZ > 0.0f) {
        if (spanA < 0.0f) { spanA += 2.0f * float(M_PI); }
    } else {
        if (spanA > 0.0f) { spanA -= 2.0f * float(M_PI); }
    }

    // Use exact corner points for the first and last arc vertices to avoid
    // sub-pixel seam gaps caused by atan2→cos/sin floating-point round-trip.
    static constexpr int ARC_STEPS = 8;
    PPoint previousPoint = outerPrev;
    for (int step = 1; step < ARC_STEPS; ++step)
    {
        const float angle = startA + spanA * float(step) / float(ARC_STEPS);
        const PPoint nextPoint = vertex + PPoint(std::cosf(angle), std::sinf(angle)) * halfWidth;
        trianglePoints.push_back(innerPoint);
        trianglePoints.push_back(previousPoint);
        trianglePoints.push_back(nextPoint);
        previousPoint = nextPoint;
    }
    trianglePoints.push_back(innerPoint);
    trianglePoints.push_back(previousPoint);
    trianglePoints.push_back(outerNext);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerView::DrawString(const PString& string)
{
    const Ptr<const PRegion> region = GetRegion();
    if (region != nullptr)
    {
        const PIPoint screenPos(m_ScreenPos);
        PIPoint penPos = PIPoint(m_PenPosition + m_ScrollOffset);
        
        PDisplayDriver* const driver = m_Bitmap->m_Driver;

        const PIRect boundingBox(penPos.x, penPos.y, penPos.x + int(driver->GetStringWidth(m_Font->Get(), string.c_str(), string.size())), penPos.y + int(driver->GetFontHeight(m_Font->Get())));

        penPos += screenPos;
        
        for (const PIRect& clip : region->m_Rects)
        {
            if (clip.DoIntersect(boundingBox)) {
                driver->WriteString(m_Bitmap, penPos, string.c_str(), string.size(), clip + screenPos, m_BgColor, m_FgColor, m_Font->Get());
            }                
        }
        m_PenPosition.x += float(boundingBox.Width());
    }        
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerView::CopyRect(const PRect& srcRect, const PPoint& dstPos)
{
    if ( m_VisibleReg == nullptr ) {
        return;
    }

    PIRect  intSrcRect(srcRect);
    const PIPoint delta   = PIPoint(dstPos) - intSrcRect.TopLeft();

    if (delta.x == 0 && delta.y == 0) {
        return;
    }

    intSrcRect += PIPoint(m_ScrollOffset);
    const PIRect  dstRect = intSrcRect + delta;

    std::vector<PIRect> bltList;
    PRegion             damage(*m_VisibleReg, intSrcRect, false);

    for (const PIRect& srcClip : m_VisibleReg->m_Rects)
    {
        // Clip to source rectangle
        PIRect sRect(intSrcRect & srcClip);

        if (!sRect.IsValid()) {
            continue;
        }
        // Transform into destination space
        sRect += delta;

        for(const PIRect& dstClip : m_VisibleReg->m_Rects)
        {
            const PIRect dRect = sRect & dstClip;

            if (!dRect.IsValid()) {
                continue;
            }
            damage.Exclude(dRect);
            bltList.push_back(dRect);
        }
    }

    
    if (bltList.empty())
    {
        Invalidate(dstRect);
        RequestPaintIfNeeded();
        return;
    }

    std::vector<PIRect*> clipList;
    clipList.reserve(bltList.size());
    
    for (PIRect& clip : bltList) {
        clipList.push_back(&clip);
    }
    std::sort(clipList.begin(), clipList.end(), BlitSortCompare(delta));

    const PIPoint screenPos(m_ScreenPos);
    for (PIRect* clip : clipList)
    {
        *clip += screenPos; // Convert into screen space
        m_Bitmap->m_Driver->CopyRect(m_Bitmap, m_Bitmap, m_BgColor, m_FgColor, *clip - delta, clip->TopLeft(), m_DrawingMode);
    }
    if (m_DamageReg != nullptr)
    {
        const PRegion region(*m_DamageReg, intSrcRect, false);
        for (const PIRect& dmgClip : region.m_Rects)
        {
            m_DamageReg->Include((dmgClip + delta)  & dstRect);
            if (m_ActiveDamageReg != nullptr) {
                m_ActiveDamageReg->Exclude((dmgClip + delta)  & dstRect);
            }
        }
    }
    if (m_ActiveDamageReg != nullptr)
    {
        const PRegion region(*m_ActiveDamageReg, intSrcRect, false);
        if (!region.IsEmpty())
        {
            if ( m_DamageReg == nullptr ) {
                m_DamageReg = ptr_new<PRegion>();
            }
            for (const PIRect& dmgClip : region.m_Rects)
            {
                m_ActiveDamageReg->Exclude((dmgClip + delta) & dstRect);
                m_DamageReg->Include((dmgClip + delta) & dstRect);
            }
        }
    }
    for (const PIRect& dstClip : damage.m_Rects) {
        Invalidate(dstClip);
    }
    if ( m_DamageReg != nullptr ) {
        m_DamageReg->Optimize();
    }
    if (m_ActiveDamageReg != nullptr) {
        m_ActiveDamageReg->Optimize();
    }
    RequestPaintIfNeeded();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerView::DrawBitmap(Ptr<PSrvBitmap> bitmap, const PRect& srcRect, const PPoint& dstPos)
{
    if (bitmap == nullptr || m_VisibleReg == nullptr) {
        return;
    }
    Ptr<const PRegion> region = GetRegion();

    if (region == nullptr) {
        return;
    }

    const PIPoint screenPos(m_ScreenPos);

    const PIRect  intSrcRect(srcRect);
    PIPoint intDstPos(dstPos + m_ScrollOffset);

    const PIRect clippedSrcRect = intSrcRect & PIRect(PIPoint(0, 0), bitmap->m_Size);
    intDstPos += clippedSrcRect.TopLeft() - intSrcRect.TopLeft();

    const PIRect  dstRect = clippedSrcRect.Bounds() + intDstPos;
    const PIPoint srcPos(clippedSrcRect.TopLeft());

    for (const PIRect& clipRect : region->m_Rects)
    {
        const PIRect rect = dstRect & clipRect;

        if (rect.IsValid())
        {
            const PIPoint cDst = rect.TopLeft() + screenPos;
            const PIRect  cSrc = rect - intDstPos + srcPos;

            m_Bitmap->m_Driver->CopyRect(m_Bitmap, ptr_raw_pointer_cast(bitmap), m_BgColor, m_FgColor, cSrc, cDst, m_DrawingMode);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerView::DrawScaledBitmap(Ptr<PSrvBitmap> bitmap, const PRect& srcRect, const PRect& inDstRect)
{
    if (bitmap == nullptr || m_VisibleReg == nullptr) {
        return;
    }
    Ptr<const PRegion> region = GetRegion();

    if (region == nullptr) {
        return;
    }
    const PIPoint screenPos(m_ScreenPos);

    const PRect snappedSrcRect = srcRect.GetFloored();
    const PRect snappedDstRect = inDstRect.GetFloored();

    const float scaleX = (snappedSrcRect.Width()  - 1.0f) / snappedDstRect.Width();
    const float scaleY = (snappedSrcRect.Height() - 1.0f) / snappedDstRect.Height();

    const float scaleXinv = 1.0f / scaleX;
    const float scaleYinv = 1.0f / scaleY;

    const PRect clippedSrcRect = snappedSrcRect & PRect::FromSize(PPoint(bitmap->m_Size));

    const PRect clippedDstRect(
        snappedDstRect.left   + (clippedSrcRect.left   - snappedSrcRect.left)   * scaleXinv,
        snappedDstRect.top    + (clippedSrcRect.top    - snappedSrcRect.top)    * scaleYinv,
        snappedDstRect.right  + (clippedSrcRect.right  - snappedSrcRect.right)  * scaleXinv,
        snappedDstRect.bottom + (clippedSrcRect.bottom - snappedSrcRect.bottom) * scaleYinv
    );

    for (const PIRect& clipRect : region->m_Rects)
    {
        const PRect rectDst = clippedDstRect & clipRect;

        if (rectDst.IsValid())
        {
            const PRect rectSrc(
                clippedSrcRect.left +   (rectDst.left   - clippedDstRect.left)   * scaleX,
                clippedSrcRect.top +    (rectDst.top    - clippedDstRect.top)    * scaleY,
                clippedSrcRect.right +  (rectDst.right  - clippedDstRect.right)  * scaleX,
                clippedSrcRect.bottom + (rectDst.bottom - clippedDstRect.bottom) * scaleY
            );

            const PIRect cDst((rectDst + m_ScreenPos).GetRounded());

            m_Bitmap->m_Driver->ScaleRect(m_Bitmap, ptr_raw_pointer_cast(bitmap), m_BgColor, m_FgColor, clippedSrcRect, clippedDstRect, rectSrc, cDst, m_DrawingMode);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerView::DebugDraw(PColor color, uint32_t drawFlags)
{
    const PIPoint screenPos(m_ScreenPos);
    
    if (drawFlags & PViewDebugDrawFlags::ViewFrame)
    {
        DebugDrawRect(GetNormalizedIBounds() + screenPos, color);
    }
    if (drawFlags & PViewDebugDrawFlags::DrawRegion)
    {
        if (m_VisibleReg != nullptr)
        {
            for (const PIRect& clip : m_VisibleReg->m_Rects)
            {
                DebugDrawRect(clip + screenPos, color);
            }
        }
    }        
    if (drawFlags & PViewDebugDrawFlags::DamageRegion)
    {
        const Ptr<PRegion> region = GetRegion();
        if (region != nullptr)
        {
            for (const PIRect& clip : region->m_Rects)
            {
                DebugDrawRect(clip + screenPos, color);
            }
        }
    }        
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerView::ScrollBy(const PPoint& offset)
{
    const Ptr<PServerView> parent = m_Parent.Lock();
    if ( parent == nullptr ) {
        return;
    }
    const PIPoint screenPos(m_ScreenPos);

    const PIPoint oldOffset(m_ScrollOffset);
    m_ScrollOffset += offset;
    const PIPoint newOffset(m_ScrollOffset);
    
    if (newOffset == oldOffset) {
        return;
    }
    UpdateScreenPos();
    const PIPoint intOffset(newOffset - oldOffset);
    
    if ( m_HideCount > 0 ) {
        return;
    }

    SetDirtyRegFlags();
    UpdateRegions();
    //SrvWindow::HandleMouseTransaction();
    
    if (m_FullReg == nullptr /*|| m_pcBitmap == nullptr*/ ) {
        return;
    }

    const PIRect        bounds(GetNormalizedBounds());
    std::vector<PIRect> bltList;
    PRegion       damage(*m_VisibleReg);

    for (const PIRect& srcClip : m_FullReg->m_Rects)
    {
        // Clip to source rectangle
        PIRect sRect = bounds & srcClip;

        // Transform into destination space
        if (!sRect.IsValid()) {
            continue;
        }
        sRect += intOffset;

        for(const PIRect& dstClip : m_FullReg->m_Rects)
        {
            const PIRect dRect = sRect & dstClip;
            if (dRect.IsValid())
            {
                damage.Exclude(dRect);
                bltList.push_back(dRect);
            }                
        }
    }
  
    if (bltList.empty())
    {
        Invalidate(bounds);
        RequestPaintIfNeeded();
        return;
    }

    std::vector<PIRect*> clipList;
    clipList.reserve(bltList.size());

    for (PIRect& clip : bltList) {
        clipList.push_back(&clip);
    }
    std::sort(clipList.begin(), clipList.end(), BlitSortCompare(intOffset));

    for (size_t i = 0 ; i < clipList.size() ; ++i)
    {
        PIRect* const clip = clipList[i];

        *clip += screenPos; // Convert into screen space

        m_Bitmap->m_Driver->CopyRect(m_Bitmap, m_Bitmap, m_BgColor, m_FgColor, *clip - intOffset, clip->TopLeft(), PDrawingMode::Copy);
    }

    if (m_DamageReg != nullptr)
    {
        for (PIRect& dstClip : m_DamageReg->m_Rects) {
            dstClip += intOffset;
        }
    }

    if (m_ActiveDamageReg != nullptr)
    {
        for (PIRect& dstClip : m_ActiveDamageReg->m_Rects) {
            dstClip += intOffset;
        }
    }
    for (const PIRect& dstClip : damage.m_Rects) {
        Invalidate(dstClip);
    }
    RequestPaintIfNeeded();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerView::UpdateScreenPos()
{
    const Ptr<PServerView> parent = m_Parent.Lock();
    if (parent == nullptr) {
        m_ScreenPos = m_Frame.TopLeft();
    } else {
        m_ScreenPos = parent->m_ScreenPos + parent->m_ScrollOffset + m_Frame.TopLeft();
    }
    for (Ptr<PServerView> child : m_ChildrenList) {
        child->UpdateScreenPos();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PServerView::DebugDrawRect(const PIRect& frame, PColor color)
{
    const PIPoint p1(frame.left, frame.top);
    const PIPoint p2(frame.right - 1, frame.top);
    const PIPoint p3(frame.right - 1, frame.bottom - 1);
    const PIPoint p4(frame.left, frame.bottom - 1);

    m_Bitmap->m_Driver->DrawLine(m_Bitmap, frame, p1, p2, color, PDrawingMode::Copy);
    m_Bitmap->m_Driver->DrawLine(m_Bitmap, frame, p2, p3, color, PDrawingMode::Copy);
    m_Bitmap->m_Driver->DrawLine(m_Bitmap, frame, p4, p3, color, PDrawingMode::Copy);
    m_Bitmap->m_Driver->DrawLine(m_Bitmap, frame, p1, p4, color, PDrawingMode::Copy);
}
