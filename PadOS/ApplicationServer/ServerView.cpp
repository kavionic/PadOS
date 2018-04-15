// This file is part of PadOS.
//
// Copyright (C) 2018 Kurt Skauen <http://kavionic.com/>
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

#include "sam.h"

#include <string.h>

#include "ServerView.h"
#include "ApplicationServer.h"

using namespace os;
using namespace kernel;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ServerView::ServerView(const String& name, const Rect& frame, const Point& scrollOffset, uint32_t flags, int32_t hideCount, Color eraseColor, Color bgColor, Color fgColor)
    : ViewBase(name, frame, scrollOffset, flags, hideCount, eraseColor, bgColor, fgColor)
    , m_EraseColor16(eraseColor.GetColor16())
    , m_BgColor16(bgColor.GetColor16())
    , m_FgColor16(fgColor.GetColor16())
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template<typename T>
struct ReverseList
{
    ReverseList(T& list) : m_List(list) {}
    
    typename T::reverse_iterator begin() { return m_List.rbegin(); }
    typename T::reverse_iterator end() { return m_List.rend(); }
    
    T& m_List;    
};

template<typename T>
struct ReverseListConst
{
    ReverseListConst(const T& list) : m_List(list) {}
    
    typename T::const_reverse_iterator begin() { return m_List.rbegin(); }
    typename T::const_reverse_iterator end() { return m_List.rend(); }
    
    const T& m_List;
};

template<typename T> ReverseList<T>      reverse_ranged(T& list) { return ReverseList<T>(list); }
template<typename T> ReverseListConst<T> reverse_ranged(const T& list) { return ReverseList<T>(list); }
    
bool ServerView::HandleMouseDown(MouseButton_e button, const Point& position)
{
    if (m_ClientHandle != -1 && !HasFlag(ViewFlags::IGNORE_MOUSE))
    {
        if (m_ManagerHandle != -1)
        {
            if (ASHandleMouseDown::Sender::Emit(get_window_manager_port(), m_ManagerHandle, INFINIT_TIMEOUT, button, position))
            {
                
            }
            else
            {
                printf("ERROR: ServerView::HandleMouseDown() failed to send message: %s\n", strerror(get_last_error()));
                return false;
            }            
        }
                    
        if (!ASHandleMouseDown::Sender::Emit(m_ClientPort, m_ClientHandle, 0, button, position)) {
            printf("ERROR: ServerView::HandleMouseDown() failed to send message: %s\n", strerror(get_last_error()));
            return false;
        }
        ApplicationServer* server = static_cast<ApplicationServer*>(GetLooper());
        if (server !=  nullptr) {
            server->SetMouseDownView(ptr_tmp_cast(this));
        }
        return true;
    }
    
    for (Ptr<ServerView> child : reverse_ranged(m_ChildrenList))
    {
        if (child->m_Frame.DoIntersect(position))
        {
            Point childPos = position - child->m_Frame.TopLeft() - child->m_ScrollOffset;
            if (child->HandleMouseDown(button, childPos))
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

bool ServerView::HandleMouseUp(MouseButton_e button, const Point& position)
{
    if (m_ClientHandle != -1 )
    {
        if (!ASHandleMouseUp::Sender::Emit(m_ClientPort, m_ClientHandle, 0, button, position)) {
            printf("ERROR: ServerView::HandleMouseUp() failed to send message: %s\n", strerror(get_last_error()));
        }
        return true;
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool ServerView::HandleMouseMove(MouseButton_e button, const Point& position)
{
    if (m_ClientHandle != -1 ) {
        if (!ASHandleMouseMove::Sender::Emit(m_ClientPort, m_ClientHandle, 0, button, position)) {
            printf("ERROR: ServerView::HandleMouseMove() failed to send message: %s\n", strerror(get_last_error()));
        }
        return true;
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerView::AddChild(Ptr<ServerView> child, bool topmost)
{
    LinkChild(child, topmost);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerView::RemoveChild(Ptr<ServerView> child)
{
    UnlinkChild(child);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerView::RemoveThis()
{
    Ptr<ServerView> parent = m_Parent.Lock();
    if (parent != nullptr)
    {
        parent->RemoveChild(ptr_tmp_cast(this));
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerView::SetFrame(const Rect& rect, handler_id requestingClient)
{
    m_Frame = rect;
    UpdateScreenPos();
    IRect cIRect(rect);
    
    Ptr<ServerView> parent = m_Parent.Lock();

    if (m_IFrame == cIRect) {
        return;
    }
    
    if (m_HideCount == 0)
    {
        m_DeltaMove += IPoint(cIRect.left, cIRect.top ) - IPoint(m_IFrame.left, m_IFrame.top);
        m_DeltaSize += IPoint(cIRect.Width(), cIRect.Height() ) - IPoint(m_IFrame.Width(), m_IFrame.Height());

        if (parent != nullptr)
        {
            for (Ptr<ServerView> i = parent; i != nullptr; i = i->GetParent())
            {
                if ((i->m_Flags & ViewFlags::TRANSPARENT) == 0) {
                    i->SetDirtyRegFlags();
                    break;
                }                    
            }

            auto i = parent->GetChildRIterator(ptr_tmp_cast(this));
            if (i != parent->m_ChildrenList.rend())
            {
                for (++i; i != parent->m_ChildrenList.rend(); ++i)
                {
                    Ptr<ServerView> sibling = *i;
                    if (sibling->m_IFrame.DoIntersect(m_IFrame) || sibling->m_IFrame.DoIntersect(cIRect))
                    {
                        sibling->MarkModified(m_IFrame - sibling->m_IFrame.TopLeft());
                        sibling->MarkModified(cIRect - sibling->m_IFrame.TopLeft());
                    }
                }                
            }
        }            
    }
    m_IFrame = cIRect;
    
    if (requestingClient != -1)
    {
        if (requestingClient == m_ClientHandle)
        {
            if (m_ManagerHandle != -1)
            {
                ApplicationServer* server = static_cast<ApplicationServer*>(GetLooper());
                if (server !=  nullptr) {
                    ASViewFrameChanged::Sender::Emit(get_window_manager_port(), m_ManagerHandle, INFINIT_TIMEOUT, m_Frame);
                }                    
            }
        }
        else
        {
            if (!ASViewFrameChanged::Sender::Emit(m_ClientPort, m_ClientHandle, 0, m_Frame)) {
                printf("ERROR: ServerView::SetFrame() failed to send message: %s\n", strerror(get_last_error()));
            }            
        }
    }
    
    if ( parent == nullptr ) {
        Invalidate();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerView::SetDrawRegion(Ptr<Region> pcReg)
{
    m_DrawConstrainReg = pcReg;

    if ( m_HideCount == 0 ) {
        m_HasInvalidRegs = true;
    }
    m_DrawReg = nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerView::SetShapeRegion(Ptr<Region> pcReg)
{
    m_ShapeConstrainReg = pcReg;

    if (m_HideCount == 0)
    {
        Ptr<ServerView> parent = m_Parent.Lock();
        if ( parent != nullptr )
        {
            for (Ptr<ServerView> i = parent; i != nullptr; i = i->GetParent())
            {
                if ((i->m_Flags & ViewFlags::TRANSPARENT) == 0) {
                    i->SetDirtyRegFlags();
                    break;
                }                    
            }
            
            auto i = parent->GetChildRIterator(ptr_tmp_cast(this));
            if (i != parent->m_ChildrenList.rend())
            {
                for (++i; i != parent->m_ChildrenList.rend(); ++i)
                {
                    Ptr<ServerView> sibling = *i;
                    if ( sibling->m_IFrame.DoIntersect(m_IFrame) ) {
                        sibling->MarkModified(m_IFrame - sibling->m_IFrame.TopLeft());
                    }
                }
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerView::Invalidate(const IRect& rect)
{
    if (m_HideCount == 0)
    {
        if (m_DamageReg == nullptr) {
            m_DamageReg = ptr_new<Region>(rect);
        } else {
            m_DamageReg->Include(rect);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerView::Invalidate(bool reqursive)
{
    if (m_HideCount == 0)
    {
        m_DamageReg = ptr_new<Region>(static_cast<IRect>(GetBounds()));
        if (reqursive)
        {
            for (Ptr<ServerView> child : m_ChildrenList) {
                child->Invalidate(true);
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerView::InvalidateNewAreas()
{
    if ( m_HideCount > 0 ) {
        return;
    }
    /*
    if (m_pcWindow != nullptr && (m_pcWindow->GetDesktopMask() & (1 << get_active_desktop())) == 0 ) {
        return;
    }*/
  
    if (m_HasInvalidRegs)
    {
        if ( ((m_Flags & ViewFlags::FULL_UPDATE_ON_H_RESIZE) && m_DeltaSize.x != 0) || ((m_Flags & ViewFlags::FULL_UPDATE_ON_V_RESIZE) && m_DeltaSize.y != 0) )
        {
            Invalidate(false);
        }
        else
        {
            if (m_VisibleReg != nullptr)
            {
                try
                {
                    Ptr<Region> region = ptr_new<Region>(*m_VisibleReg);
    
                    if ( m_PrevVisibleReg != nullptr ) {
                        region->Exclude( *m_PrevVisibleReg );
                    }
                    if (m_DamageReg == nullptr)
                    {
                        if ( region->IsEmpty() == false ) {
                            m_DamageReg = region;
                        }
                    }
                    else
                    {
                        ENUMCLIPLIST(&region->m_cRects, clip) {
                            Invalidate(clip->m_cBounds);
                        }
                    }
                }
                catch(const std::bad_alloc&) {}
            }
        }
        m_PrevVisibleReg = nullptr;

        m_DeltaSize = IPoint( 0, 0 );
        m_DeltaMove = IPoint( 0, 0 );
    }
  
    for (Ptr<ServerView> child : m_ChildrenList) {
        child->InvalidateNewAreas();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static int SortCmp(const void* pNode1, const void* pNode2)
{
    ClipRect* clip1 = *((ClipRect**)pNode1);
    ClipRect* clip2 = *((ClipRect**)pNode2);


    if ( clip1->m_cBounds.left > clip2->m_cBounds.right && clip1->m_cBounds.right < clip2->m_cBounds.left )
    {
        if ( clip1->m_cMove.x < 0 ) {
            return clip1->m_cBounds.left - clip2->m_cBounds.left;
            } else {
            return clip2->m_cBounds.left - clip1->m_cBounds.left;
        }
    }
    else
    {
        if ( clip1->m_cMove.y < 0 ) {
            return clip1->m_cBounds.top - clip2->m_cBounds.top;
            } else {
            return clip2->m_cBounds.top - clip1->m_cBounds.top;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerView::MoveChilds()
{
    if ( m_HideCount > 0  /*|| nullptr == m_pcBitmap */) {
        return;
    }
/*    if (m_pcWindow != nullptr && (m_pcWindow->GetDesktopMask() & (1 << get_active_desktop())) == 0) {
        return;
    }*/
  
    if ( m_HasInvalidRegs )
    {
        Rect cBounds = GetBounds();
        IRect cIBounds( cBounds );
        IPoint screenPos(m_ScreenPos);
        for (Ptr<ServerView> child : m_ChildrenList)
        {
            if (child->m_DeltaMove.x == 0 && child->m_DeltaMove.y == 0) {
                continue;
            }
            if (child->m_FullReg == nullptr || child->m_PrevFullReg == nullptr) {
                continue;
            }

            Ptr<Region> region = ptr_new<Region>(*child->m_PrevFullReg);
            region->Intersect(*child->m_FullReg);

            int count = 0;
    
            IPoint cChildOffset = IPoint(child->m_ScreenPos);
            IPoint cChildMove(child->m_DeltaMove);
            ENUMCLIPLIST(&region->m_cRects, clip)
            {
                // Transform into parents coordinate system
                clip->m_cBounds += cChildOffset;
                clip->m_cMove    = cChildMove;
                count++;
                assert(clip->m_cBounds.IsValid());
            }
            if ( count == 0 ) {
                continue;
            }

            ClipRect** apsClips = new ClipRect*[count];

            for ( int i = 0 ; i < count ; ++i ) {
                apsClips[i] = region->m_cRects.RemoveHead();
                assert(apsClips[i] != nullptr);
            }
            qsort(apsClips, count, sizeof( ClipRect* ), SortCmp);

            for ( int i = 0 ; i < count ; ++i )
            {
                ClipRect* clip = apsClips[i];

                GfxDriver::Instance.BLT_MoveRect(clip->m_cBounds - clip->m_cMove, clip->m_cBounds.TopLeft());
                m_VisibleReg->FreeClipRect(clip);
            }
            delete[] apsClips;
        }

        // Since the parent window is shrinked before the children is moved
        // we may need to redraw the right and bottom edges.

        Ptr<ServerView> parent = m_Parent.Lock();
        if ( parent != nullptr && (m_DeltaMove.x != 0.0f || m_DeltaMove.y != 0.0f) )
        {
            if ( parent->m_DeltaSize.x < 0 ) {
                IRect cRect = cIBounds;

                cRect.left = cRect.right + int(parent->m_DeltaSize.x + parent->m_IFrame.right - parent->m_IFrame.left - m_IFrame.right);

                if ( cRect.IsValid() ) {
                    Invalidate( cRect );
                }
            }
            if ( parent->m_DeltaSize.y < 0 ) {
                IRect cRect = cIBounds;

                cRect.top = cRect.bottom + int(parent->m_DeltaSize.y + parent->m_IFrame.bottom - parent->m_IFrame.top - m_IFrame.bottom);

                if ( cRect.IsValid() ) {
                    Invalidate(cRect);
                }
            }
        }
        m_PrevFullReg = nullptr;
    }
    for (Ptr<ServerView> child : m_ChildrenList) {
        child->MoveChilds();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// DESC:
///     Stores the previous visible region in m_PrevVisibleReg and then
///     rebuilds m_VisibleReg, starting with whatever is left of our parent
///     and then removing areas covered by siblings.
/// NOTE:
///     Areas covered by children are not removed.
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerView::SwapRegions( bool bForce )
{
    if ( bForce ) {
        m_HasInvalidRegs = true;
    }
  
    for (Ptr<ServerView> child : m_ChildrenList) {
        child->SwapRegions( bForce );
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerView::RebuildRegion( bool bForce )
{
/*    if (m_pcWindow != nullptr && (m_pcWindow->GetDesktopMask() & (1 << get_active_desktop())) == 0) {
        return;
    }*/

    if ( m_HideCount > 0 )
    {
        if ( m_VisibleReg != nullptr ) {
            DeleteRegions();
        }
        return;
    }
  
    if ( bForce ) {
        m_HasInvalidRegs = true;
    }
  
    if (m_HasInvalidRegs)
    {
        m_DrawReg = nullptr;

        assert(m_PrevVisibleReg == nullptr);
        assert(m_PrevFullReg == nullptr);
    
        m_PrevVisibleReg = m_VisibleReg;
        m_PrevFullReg    = m_FullReg;

        Ptr<ServerView> parent = m_Parent.Lock();
        if ( parent == nullptr )
        {
            m_FullReg = ptr_new<Region>(m_IFrame);
        }
        else
        {
            assert(parent->m_FullReg != nullptr);
            if ( parent->m_FullReg == nullptr ) {
                m_FullReg = ptr_new<Region>(m_IFrame.Bounds());
            } else {
                m_FullReg = ptr_new<Region>(*parent->m_FullReg, m_IFrame, true);
            }
            if ( m_ShapeConstrainReg != nullptr ) {
                m_FullReg->Intersect(*m_ShapeConstrainReg);
            }
            IPoint topLeft(m_IFrame.TopLeft());
            auto i = parent->GetChildIterator(ptr_tmp_cast(this));
            if (i != parent->m_ChildrenList.end())
            {
                for (++i; i != parent->m_ChildrenList.end(); ++i) // Loop over all siblings above us.
                {
                    Ptr<ServerView> sibling = *i;
                    if (sibling->m_HideCount == 0)
                    {
                        if (sibling->m_IFrame.DoIntersect(m_IFrame))
                        {
                            sibling->ExcludeFromRegion(m_FullReg, -topLeft);
                        }
                    }
                }
            }                
            m_FullReg->Optimize();
        }            
        m_VisibleReg = ptr_new<Region>(*m_FullReg);

        if ( (m_Flags & ViewFlags::DRAW_ON_CHILDREN) == 0 )
        {
            bool regModified = false;
            IPoint scrollOffset(m_ScrollOffset);
            for (Ptr<ServerView> child : m_ChildrenList)
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
    for (Ptr<ServerView> child : m_ChildrenList) {
        child->RebuildRegion(bForce);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool ServerView::ExcludeFromRegion(Ptr<Region> region, const IPoint& offset)
{
    if (m_HideCount == 0)
    {
        if ((m_Flags & ViewFlags::TRANSPARENT) == 0)
        {
//            IRect r = m_IFrame + offset;
//            printf("Exclude %s: %d, %d, %d, %d\n", GetName().c_str(), r.left, r.top, r.right, r.bottom);
            if ( m_ShapeConstrainReg == nullptr ) {
                region->Exclude(m_IFrame + offset);
            } else {
                region->Exclude(*m_ShapeConstrainReg, m_IFrame.TopLeft() + offset);
            }
            return true;
        }
        else
        {
//            printf("View %s is transparent. Excluding childrens (%d, %d)\n", GetName().c_str(), offset.x, offset.y);
            bool wasModified = false;
            IPoint framePos = m_IFrame.TopLeft();
            IPoint scrollOffset(m_ScrollOffset);
            for (Ptr<ServerView> child : m_ChildrenList)
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

void ServerView::ClearDirtyRegFlags()
{
    m_HasInvalidRegs = false;
    for (Ptr<ServerView> child : m_ChildrenList) {
        child->ClearDirtyRegFlags();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerView::UpdateRegions(bool bForce, bool bRoot)
{
    RebuildRegion( bForce );
    MoveChilds();
    InvalidateNewAreas();

    ApplicationServer* server = static_cast<ApplicationServer*>(GetLooper());
    if (server !=  nullptr)
    {
        if (m_HasInvalidRegs && server->GetTopView() == this /*m_pcBitmap != nullptr*/ && m_DamageReg != nullptr)
        {
//      if ( m_pcBitmap == g_pcScreenBitmap )
            {
                Region cDrawReg(*m_VisibleReg);
                cDrawReg.Intersect(*m_DamageReg);
        
                GfxDriver::Instance.SetFgColor(m_EraseColor16);
                IPoint screenPos(m_ScreenPos);
                ENUMCLIPLIST(&cDrawReg.m_cRects, clip) {
                    GfxDriver::Instance.FillRect(clip->m_cBounds + screenPos);
                }
            }
            m_DamageReg = nullptr;
        }
    }        
    UpdateIfNeeded(false);
    ClearDirtyRegFlags();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerView::DeleteRegions()
{
    assert(m_PrevVisibleReg == nullptr);
    assert(m_PrevFullReg == nullptr);

    m_VisibleReg      = nullptr;
    m_FullReg         = nullptr;
    m_DrawReg         = nullptr;
    m_DamageReg       = nullptr;
    m_ActiveDamageReg = nullptr;
    
    for (Ptr<ServerView> child : m_ChildrenList) {
        child->DeleteRegions();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<Region> ServerView::GetRegion()
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
                m_DrawReg = ptr_new<Region>(*m_VisibleReg);
                m_DrawReg->Intersect( *m_DrawConstrainReg );
            }
        }
    }
    else if (m_DrawReg == nullptr && m_VisibleReg != nullptr)
    {
        m_DrawReg = ptr_new<Region>(*m_VisibleReg);

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

void ServerView::ToggleDepth()
{
    Ptr<ServerView> self = ptr_tmp_cast(this);
    Ptr<ServerView> parent = GetParent();
    if ( parent != NULL )
    {
        if (parent->m_ChildrenList[parent->m_ChildrenList.size()-1] == self)
        {
            parent->RemoveChild(self);
            parent->AddChild(self, false);
        }
        else
        {
            parent->RemoveChild(self);
            parent->AddChild(self, true);
        }

//        parent->m_bHasInvalidRegs = true;
//        SetDirtyRegFlags();
    
        Ptr<ServerView> opacParent = GetOpacParent(parent, nullptr);
        
        opacParent->SetDirtyRegFlags();
    
//        Layer* pcSibling;
//        for ( pcSibling = m_pcParent->m_pcBottomChild ; pcSibling != NULL ; pcSibling = pcSibling->m_pcHigherSibling )
        for (Ptr<ServerView> sibling : *parent)
        {
            if (sibling->m_IFrame.DoIntersect(m_IFrame)) {
                sibling->MarkModified(m_IFrame - sibling->m_IFrame.TopLeft());
            }
        }
        opacParent->UpdateRegions(false);
        //ServerApplication* server = ptr_static_cast<ApplicationServer>(GetLooper());
        //server->Update
    }
//    return false;

}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerView::BeginUpdate()
{
    if (m_VisibleReg != nullptr) {
        m_IsUpdating = true;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerView::EndUpdate( void )
{
    m_ActiveDamageReg = nullptr;
    m_DrawReg         = nullptr;
    
    m_IsUpdating = false;
    
    if (m_DamageReg != nullptr)
    {
        m_ActiveDamageReg = m_DamageReg;
        m_DamageReg = nullptr;
        Paint(static_cast<Rect>(m_ActiveDamageReg->GetBounds()));
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerView::Paint(const IRect& updateRect)
{
    if ( m_HideCount > 0 || m_IsUpdating == true || m_ClientHandle == -1) {
        return;
    }
    if (!ASPaintView::Sender::Emit(m_ClientPort, m_ClientHandle, 0, updateRect)) {
        printf("ERROR: ServerView::Paint() failed to send message: %s\n", strerror(get_last_error()));        
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerView::UpdateIfNeeded(bool force)
{
    if ( m_HideCount > 0) {
        return;
    }
    //    if ( m_pcWindow != nullptr && ((m_pcWindow->GetDesktopMask() & (1 << get_active_desktop())) == 0 ||
    //                              m_pcWindow->HasPendingSizeEvents( this )) ) {
    //      return;
    //    }
    
    if (m_DamageReg != nullptr)
    {
        if (m_ActiveDamageReg == nullptr)
        {
            m_ActiveDamageReg = m_DamageReg;
            m_DamageReg = nullptr;
            m_ActiveDamageReg->Optimize();
            Paint( static_cast<Rect>(m_ActiveDamageReg->GetBounds()));
        }
    }
    for (Ptr<ServerView> child : m_ChildrenList) {
        child->UpdateIfNeeded(force);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerView::MarkModified(const IRect& rect)
{
    if ( GetBounds().DoIntersect( rect ) )
    {
        m_HasInvalidRegs = true;
        IPoint scrollOffset(m_ScrollOffset);
        
        for (Ptr<ServerView> child : m_ChildrenList) {
            child->MarkModified(rect - child->m_IFrame.TopLeft() - scrollOffset);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerView::SetDirtyRegFlags()
{
    m_HasInvalidRegs = true;
    
    for (Ptr<ServerView> child : m_ChildrenList) {
        child->SetDirtyRegFlags();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerView::Show(bool doShow)
{
    //    if ( m_pcParent == nullptr || m_pcWindow == nullptr ) {
    //      dbprintf( "Error: Layer::Show() attempt to hide root layer\n" );
    //      return;
    //    }
    if ( doShow ) {
        m_HideCount--;
    } else {
        m_HideCount++;
    }

    Ptr<ServerView> parent = m_Parent.Lock();
    if (parent != nullptr)
    {
        for (Ptr<ServerView> i = parent; i != nullptr; i = i->GetParent())
        {
            if ((i->m_Flags & ViewFlags::TRANSPARENT) == 0) {
                i->SetDirtyRegFlags();
                break;
            }                    
        }
        for (Ptr<ServerView> sibling : parent->m_ChildrenList)
        {
            if (sibling->m_IFrame.DoIntersect(m_IFrame)) {
                sibling->MarkModified(m_IFrame - sibling->m_IFrame.TopLeft());
            }
        }
    }
    for (auto child = m_ChildrenList.rbegin(); child != m_ChildrenList.rend(); ++child) {
        (*child)->Show(doShow);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerView::DrawLineTo(const Point& toPoint)
{
    DrawLine(m_PenPosition, toPoint);
    m_PenPosition = toPoint;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerView::DrawLine(const Point& fromPnt, const Point& toPnt )
{
    Ptr<const Region> region = GetRegion();
    if (region != nullptr)
    {
        IPoint screenPos(m_ScreenPos);
        IPoint fromPntScr(fromPnt + m_ScrollOffset);
        IPoint toPntScr(toPnt + m_ScrollOffset);
        fromPntScr += screenPos;
        toPntScr   += screenPos;
        
        IRect screenFrame = ApplicationServer::GetScreenIFrame();

        if (!Region::ClipLine(screenFrame, &fromPntScr, &toPntScr)) return;

        if (fromPntScr.x > toPntScr.x)
        {
            std::swap(toPntScr, fromPntScr);
        }
        
        GfxDriver::Instance.WaitBlitter();
        GfxDriver::Instance.SetWindow(ApplicationServer::GetScreenFrame());
        GfxDriver::Instance.SetFgColor(m_FgColor16);
        bool first = true;
        ENUMCLIPLIST(&region->m_cRects, clip)
        {
            IPoint p1 = fromPntScr;
            IPoint p2 = toPntScr;
            if (!Region::ClipLine(clip->m_cBounds + screenPos, &p1, &p2)) continue;

            if (!first) {
                GfxDriver::Instance.WaitBlitter();
            } else {
                first = false;
            }
            GfxDriver::Instance.DrawLine(p1.x, p1.y, p2.x, p2.y);
//            GfxDriver::Instance.SetWindow(clip->m_cBounds + screenPos);
//            GfxDriver::Instance.DrawLine(fromPntScr.x, fromPntScr.y, toPntScr.x, toPntScr.y);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerView::FillRect(const Rect& rect, Color color)
{
    Ptr<const Region> region = GetRegion();
    if (region != nullptr)
    {
        GfxDriver::Instance.WaitBlitter();
        GfxDriver::Instance.SetFgColor(color.GetColor16());
        IPoint screenPos(m_ScreenPos);
        IRect rectScr(rect + m_ScrollOffset);
        GfxDriver::Instance.SetWindow(ApplicationServer::GetScreenFrame());
    
        bool first = true;
        ENUMCLIPLIST(&region->m_cRects, clip)
        {
            if (!first) {
                GfxDriver::Instance.WaitBlitter();
            } else {
                first = false;
            }
            IRect clippedRect(rectScr & clip->m_cBounds);
            if (clippedRect.IsValid()) {
                clippedRect += screenPos;
                GfxDriver::Instance.FillRect(clippedRect);
            }
        }
    }        
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerView::FillCircle(const Point& position, float radius)
{
    if (int(position.y + m_ScreenPos.y + radius) >= 510) return; // Broken clipping past that.
    
    Ptr<const Region> region = GetRegion();
    if (region != nullptr)
    {
        IPoint screenPos(m_ScreenPos);
        IPoint positionScr(position + m_ScrollOffset);
        int    radiusRounded = int(radius + 0.5f);
        
        positionScr += screenPos;
        
        GfxDriver::Instance.WaitBlitter();
        GfxDriver::Instance.SetFgColor(m_FgColor16);
        bool first = true;
        IRect boundingBox(positionScr.x - radiusRounded + 2, positionScr.y - radiusRounded + 2, positionScr.x + radiusRounded - 2, positionScr.y + radiusRounded - 2);
        ENUMCLIPLIST(&region->m_cRects, clip)
        {
            IRect clipRect = clip->m_cBounds + screenPos;
            if (!boundingBox.DoIntersect(clipRect)) {
                continue;
            }
            if (!first) {
                GfxDriver::Instance.WaitBlitter();
            } else {
                first = false;
            }
            GfxDriver::Instance.SetWindow(clipRect);
            GfxDriver::Instance.FillCircle(positionScr.x, positionScr.y, radiusRounded);
        }
    }    
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerView::DrawString(const String& string, float maxWidth, uint8_t flags)
{
    Ptr<const Region> region = GetRegion();
    if (region != nullptr)
    {
        GfxDriver::Instance.WaitBlitter();
        GfxDriver::Instance.SetFont(m_Font->Get());
        GfxDriver::Instance.SetBgColor(m_BgColor16);
        GfxDriver::Instance.SetFgColor(m_FgColor16);

        IPoint screenPos(m_ScreenPos);
        IPoint penPos = screenPos + IPoint(m_PenPosition + m_ScrollOffset);
        ENUMCLIPLIST(&region->m_cRects, clip)
        {
            GfxDriver::Instance.SetCursor(penPos);
            GfxDriver::Instance.WriteString(string.c_str(), string.size(), clip->m_cBounds + screenPos);
        }
        m_PenPosition = Point(GfxDriver::Instance.GetCursor() - screenPos);
    }        
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerView::CopyRect(const Rect& srcRect, const Point& dstPos)
{
    if ( m_VisibleReg == nullptr ) {
        return;
    }

    IRect  intSrcRect(srcRect);
    IPoint delta   = IPoint(dstPos) - intSrcRect.TopLeft();
    IRect  dstRect = intSrcRect + delta;

    ClipRectList bltList;
    Region       damage(*m_VisibleReg, dstRect, false);

    ENUMCLIPLIST(&m_VisibleReg->m_cRects, srcClip)
    {
        // Clip to source rectangle
        IRect sRect(intSrcRect & srcClip->m_cBounds);

        if (!sRect.IsValid()) {
            continue;
        }
        // Transform into destination space
        sRect += delta;

        ENUMCLIPLIST( &m_VisibleReg->m_cRects, dstClip )
        {
            IRect dRect = sRect & dstClip->m_cBounds;

            if (!dRect.IsValid()) {
                continue;
            }
            damage.Exclude(dRect);
            ClipRect* clip  = Region::AllocClipRect();
            clip->m_cBounds = dRect;
            clip->m_cMove   = delta;

            bltList.AddRect(clip);
        }
    }

    int count = bltList.GetCount();
    
    if (count == 0)
    {
        Invalidate(dstRect);
        UpdateIfNeeded(true);
        return;
    }

    ClipRect** apsClips = new ClipRect*[count];

    for ( int i = 0 ; i < count ; ++i )
    {
        apsClips[i] = bltList.RemoveHead();
        assert(apsClips[i] != nullptr);
    }
    qsort(apsClips, count, sizeof( ClipRect* ), SortCmp);

    IPoint screenPos(m_ScreenPos);
    for ( int i = 0 ; i < count ; ++i )
    {
        ClipRect* clip = apsClips[i];
        clip->m_cBounds += screenPos; // Convert into screen space
        GfxDriver::Instance.BLT_MoveRect(clip->m_cBounds - clip->m_cMove, clip->m_cBounds.TopLeft());
        Region::FreeClipRect(clip);
    }
    delete[] apsClips;
    if (m_DamageReg != nullptr)
    {
        Region cReg(*m_DamageReg, intSrcRect, false);
        ENUMCLIPLIST( &cReg.m_cRects, pcDmgClip )
        {
            m_DamageReg->Include((pcDmgClip->m_cBounds + delta)  & dstRect);
            if (m_ActiveDamageReg != nullptr) {
                m_ActiveDamageReg->Exclude((pcDmgClip->m_cBounds + delta)  & dstRect);
            }
        }
    }
    if (m_ActiveDamageReg != nullptr)
    {
        Region cReg(*m_ActiveDamageReg, intSrcRect, false);
        if (cReg.m_cRects.GetCount() > 0)
        {
            if ( m_DamageReg == nullptr ) {
                m_DamageReg = ptr_new<Region>();
            }
            ENUMCLIPLIST( &cReg.m_cRects, pcDmgClip )
            {
                m_ActiveDamageReg->Exclude((pcDmgClip->m_cBounds + delta) & dstRect);
                m_DamageReg->Include((pcDmgClip->m_cBounds + delta) & dstRect);
            }
        }
    }
    ENUMCLIPLIST( &damage.m_cRects, dstClip ) {
        Invalidate(dstClip->m_cBounds);
    }
    if ( m_DamageReg != nullptr ) {
        m_DamageReg->Optimize();
    }
    if (m_ActiveDamageReg != nullptr) {
        m_ActiveDamageReg->Optimize();
    }
    UpdateIfNeeded(true);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerView::ScrollBy(const Point& offset)
{
    Ptr<ServerView> parent = m_Parent.Lock();
    if ( parent == nullptr ) {
        return;
    }
    IPoint screenPos(m_ScreenPos);

    IPoint oldOffset(m_ScrollOffset);
    m_ScrollOffset += offset;
    IPoint newOffset(m_ScrollOffset);
    
    if ( newOffset == oldOffset ) {
        return;
    }
    UpdateScreenPos();
    IPoint intOffset(newOffset - oldOffset);
    
//    printf("ScrollBy(%.2f, %.2f) -> %.2f, %.2f\n", offset.x, offset.y, m_ScrollOffset.x, m_ScrollOffset.y);
    
    if ( m_HideCount > 0 ) {
        return;
    }

    UpdateRegions();
    //SrvWindow::HandleMouseTransaction();
    
    if (m_FullReg == nullptr /*|| m_pcBitmap == nullptr*/ ) {
        return;
    }

    Rect         cBounds = GetBounds();
    IRect        cIBounds(cBounds);
    ClipRectList bltList;
    Region       damage(*m_VisibleReg);

    ENUMCLIPLIST(&m_FullReg->m_cRects, srcClip)
    {
        // Clip to source rectangle
        IRect sRect = cIBounds & srcClip->m_cBounds;

        // Transform into destination space
        if (!sRect.IsValid()) {
            continue;
        }
        sRect += intOffset;

        ENUMCLIPLIST(&m_FullReg->m_cRects, dstClip)
        {
            IRect dRect = sRect & dstClip->m_cBounds;

            if (!dRect.IsValid()) {
                continue;
            }
            damage.Exclude(dRect);

            ClipRect* clip = Region::AllocClipRect();
            clip->m_cBounds = dRect;
            clip->m_cMove   = intOffset;

            bltList.AddRect(clip);
        }
    }
  
    int count = bltList.GetCount();
    
    if (count == 0)
    {
        Invalidate(cIBounds);
        UpdateIfNeeded(true);
        return;
    }

    ClipRect** apsClips = new ClipRect*[count];

    for (int i = 0 ; i < count ; ++i)
    {
        apsClips[i] = bltList.RemoveHead();
        assert( apsClips[i] != nullptr );
    }
    qsort(apsClips, count, sizeof(ClipRect*), SortCmp);

    for (int i = 0 ; i < count ; ++i)
    {
        ClipRect* clip = apsClips[i];

        clip->m_cBounds += screenPos; // Convert into screen space

        GfxDriver::Instance.BLT_MoveRect(clip->m_cBounds - clip->m_cMove, clip->m_cBounds.TopLeft());
        Region::FreeClipRect(clip);
    }
    delete[] apsClips;

    if (m_DamageReg != nullptr)
    {
        ENUMCLIPLIST(&m_DamageReg->m_cRects, dstClip) {
            dstClip->m_cBounds += intOffset;
        }
    }

    if (m_ActiveDamageReg != nullptr)
    {
        ENUMCLIPLIST(&m_ActiveDamageReg->m_cRects, dstClip) {
            dstClip->m_cBounds += intOffset;
        }
    }
    ENUMCLIPLIST(&damage.m_cRects, dstClip) {
        Invalidate(dstClip->m_cBounds);
    }
    UpdateIfNeeded(true);
}
