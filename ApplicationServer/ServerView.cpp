// This file is part of PadOS.
//
// Copyright (C) 2018-2021 Kurt Skauen <http://kavionic.com/>
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

#include <string.h>

#include <ApplicationServer/ApplicationServer.h>
#include <ApplicationServer/DisplayDriver.h>
#include <ApplicationServer/ServerBitmap.h>
#include <Utils/Utils.h>

#include "ServerView.h"


static int g_ServerViewCount = 0;

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
    PFontID              fontID,
    PColor               eraseColor,
    PColor               bgColor,
    PColor               fgColor
)
    : PViewBase(name, frame, scrollOffset, flags, hideCount, penWidth, eraseColor, bgColor, fgColor)
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
        
            const PIPoint screenPos(m_ScreenPos);
            for (const PIRect& clip : cDrawReg.m_Rects) {
                m_Bitmap->m_Driver->FillRect(m_Bitmap, clip + screenPos, m_EraseColor);
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
    if (m_PenWidth < 2.0f)
    {
        DrawThinLine(fromPnt, toPnt);
    }
    else
    {
        const PPoint start = fromPnt.GetRounded();
        const PPoint end = toPnt.GetRounded();
        const float halfThickness = m_PenWidth * 0.5f;

        const PPoint delta = end - start;
        const PPoint normal = PPoint(-delta.y, delta.x).GetNormalized();

        PPoint prevOffset = (normal * -halfThickness).GetRounded();
        DrawThinLine(start + prevOffset, end + prevOffset);

        for (int32_t i = 1; i < int32_t(std::round(m_PenWidth)); ++i)
        {
            PPoint offset = normal * (float(i) - halfThickness);
            const PPoint offsetDelta = offset - prevOffset;
            if (std::abs(offsetDelta.x) > std::abs(offsetDelta.y))
            {
                offset.x = (offsetDelta.x < 0.0f) ? (prevOffset.x - 1.0f) : (prevOffset.x + 1.0f);
                offset.y = prevOffset.y;
            }
            else
            {
                offset.x = prevOffset.x;
                offset.y = (offsetDelta.y < 0.0f) ? (prevOffset.y - 1.0f) : (prevOffset.y + 1.0f);
            }
            prevOffset = offset;
            DrawThinLine(start + offset, end + offset);
        }
    }
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
        
        for (const PIRect& clip : region->m_Rects)
        {
            PIRect clippedRect(rectScr & clip);
            if (clippedRect.IsValid()) {
                clippedRect += screenPos;
                m_Bitmap->m_Driver->FillRect(m_Bitmap, clippedRect, color);
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
            if (clip.DoIntersect(boundingBox))
            {
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
