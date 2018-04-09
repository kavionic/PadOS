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

bool ServerView::HandleMouseDown(MouseButton_e button, const Point& position)
{
    if (m_ClientHandle != -1 ) {
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
    
    for (Ptr<ServerView> child : m_ChildrenList)
    {
        if (child->m_Frame.DoIntersect(position))
        {
            Point childPos = position - child->m_Frame.LeftTop() - m_ScrollOffset;
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

void ServerView::AddChild(Ptr<ServerView> child)
{
    LinkChild(child, true);
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

void ServerView::SetFrame(const Rect& rect)
{
    m_Frame = rect;
    UpdateScreenPos();
    IRect cIRect(rect);
    
    Ptr<ServerView> parent = m_Parent.Lock();
    
    if (m_HideCount == 0)
    {
        if (m_IFrame == cIRect) {
            return;
        }
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
                        sibling->MarkModified(m_IFrame - sibling->m_IFrame.LeftTop());
                        sibling->MarkModified(cIRect - sibling->m_IFrame.LeftTop());
                    }
                }                
            }
        }            
    }
    m_IFrame = cIRect;
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
                        sibling->MarkModified(m_IFrame - sibling->m_IFrame.LeftTop());
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
                        ENUMCLIPLIST( &region->m_cRects, pcClip ) {
                            Invalidate( pcClip->m_cBounds );
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
    ClipRect* pcClip1 = *((ClipRect**)pNode1);
    ClipRect* pcClip2 = *((ClipRect**)pNode2);


    if ( pcClip1->m_cBounds.left > pcClip2->m_cBounds.right && pcClip1->m_cBounds.right < pcClip2->m_cBounds.left )
    {
        if ( pcClip1->m_cMove.x < 0 ) {
            return pcClip1->m_cBounds.left - pcClip2->m_cBounds.left;
            } else {
            return pcClip2->m_cBounds.left - pcClip1->m_cBounds.left;
        }
    }
    else
    {
        if ( pcClip1->m_cMove.y < 0 ) {
            return pcClip1->m_cBounds.top - pcClip2->m_cBounds.top;
            } else {
            return pcClip2->m_cBounds.top - pcClip1->m_cBounds.top;
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
        for (Ptr<ServerView> child : m_ChildrenList)
        {
            if ( child->m_DeltaMove.x == 0.0f && child->m_DeltaMove.y == 0.0f ) {
                continue;
            }
            if ( child->m_FullReg == nullptr || child->m_PrevFullReg == nullptr ) {
                continue;
            }

            Ptr<Region> region = ptr_new<Region>(*child->m_PrevFullReg);
            region->Intersect( *child->m_FullReg );

            int nCount = 0;
    
            IPoint cTopLeft( ConvertToRoot( Point( 0, 0 ) ) );

            IPoint cChildOffset(IPoint(child->m_IFrame.left, child->m_IFrame.top) + cTopLeft);
            IPoint cChildMove( child->m_DeltaMove );
            ENUMCLIPLIST(&region->m_cRects, pcClip)
            {
                // Transform into parents coordinate system
                pcClip->m_cBounds += cChildOffset;
                pcClip->m_cMove    = cChildMove;
                nCount++;
                assert(pcClip->m_cBounds.IsValid());
            }
            if ( nCount == 0 ) {
                continue;
            }

            ClipRect** apsClips = new ClipRect*[nCount];

            for ( int i = 0 ; i < nCount ; ++i ) {
                apsClips[i] = region->m_cRects.RemoveHead();
                assert(apsClips[i] != nullptr);
            }
            qsort( apsClips, nCount, sizeof( ClipRect* ), SortCmp );

            for ( int i = 0 ; i < nCount ; ++i ) {
                ClipRect* pcClip = apsClips[i];

                GfxDriver::Instance.BLT_MoveRect(pcClip->m_cBounds - pcClip->m_cMove, IPoint( pcClip->m_cBounds.left, pcClip->m_cBounds.top ));
                m_VisibleReg->FreeClipRect(pcClip);
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
                m_FullReg = ptr_new<Region>(m_IFrame);
            } else {
                m_FullReg = ptr_new<Region>(*parent->m_FullReg, m_IFrame, true);
            }
            if ( m_ShapeConstrainReg != nullptr ) {
                m_FullReg->Intersect( *m_ShapeConstrainReg );
            }
            IPoint cLeftTop(m_IFrame.LeftTop());
            auto i = parent->GetChildIterator(ptr_tmp_cast(this));
            if (i != parent->m_ChildrenList.end())
            {
                for (++i; i != parent->m_ChildrenList.end(); ++i)
                {
                    Ptr<ServerView> sibling = *i;
                    if ( sibling->m_HideCount == 0 )
                    {
                        if (sibling->m_IFrame.DoIntersect(m_IFrame))
                        {
                            if (sibling->m_ShapeConstrainReg == nullptr) {
                                m_FullReg->Exclude(sibling->m_IFrame - cLeftTop);
                            } else {
                                m_FullReg->Exclude(*sibling->m_ShapeConstrainReg, sibling->m_IFrame.LeftTop() - cLeftTop);
                            }
                        }
                    }
                }
            }                
        }
        m_FullReg->Optimize();
        m_VisibleReg = ptr_new<Region>(*m_FullReg);

        if ( (m_Flags & ViewFlags::DRAW_ON_CHILDREN) == 0 )
        {
            bool regModified = false;
            for (Ptr<ServerView> child : m_ChildrenList)
            {
                // Remove children from child region
                if (child->ExcludeFromRegion(m_VisibleReg, IPoint(0, 0))) {
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
                region->Exclude(*m_ShapeConstrainReg, m_IFrame.LeftTop() + offset);
            }
            return true;
        }
        else
        {
//            printf("View %s is transparent. Excluding childrens (%d, %d)\n", GetName().c_str(), offset.x, offset.y);
            bool wasModified = false;
            IPoint framePos = m_IFrame.LeftTop();
            for (Ptr<ServerView> child : m_ChildrenList)
            {
                if (child->ExcludeFromRegion(region, offset + framePos)) {
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

    if (m_HasInvalidRegs && m_Parent.Lock() == nullptr /*m_pcBitmap != nullptr*/ && m_DamageReg != nullptr)
    {
//      if ( m_pcBitmap == g_pcScreenBitmap )
        {
            uint16_t color = LCD_RGB( 0x00, 0x60, 0x6b);
            IPoint    cTopLeft( ConvertToRoot( Point( 0, 0 ) ) );

            Region cDrawReg(*m_VisibleReg);
            cDrawReg.Intersect(*m_DamageReg);
        
            GfxDriver::Instance.SetFgColor(color);
            ENUMCLIPLIST( &cDrawReg.m_cRects, pcClip ) {
                GfxDriver::Instance.FillRect(pcClip->m_cBounds + cTopLeft);
            }
        }
        m_DamageReg = nullptr;
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
        
        for (Ptr<ServerView> child : m_ChildrenList) {
            child->MarkModified(rect - child->m_IFrame.LeftTop());
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
                sibling->MarkModified(m_IFrame - sibling->m_IFrame.LeftTop());
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

        if (!Region::ClipLine(screenFrame, &fromPntScr.x, &fromPntScr.y, &toPntScr.x, &toPntScr.y)) return;

        if (fromPntScr.x > toPntScr.x)
        {
            std::swap(toPntScr, fromPntScr);
        }
        
        GfxDriver::Instance.WaitBlitter();
        GfxDriver::Instance.SetFgColor(m_FgColor16);
        bool first = true;
        ENUMCLIPLIST(&region->m_cRects, clip)
        {
            if (!first) {
                GfxDriver::Instance.WaitBlitter();
                } else {
                first = false;
            }
            GfxDriver::Instance.SetWindow(clip->m_cBounds + screenPos);
            GfxDriver::Instance.DrawLine(fromPntScr.x, fromPntScr.y, toPntScr.x, toPntScr.y);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerView::FillRect(const Rect& rect)
{
    Ptr<const Region> region = GetRegion();
    if (region != nullptr)
    {
        GfxDriver::Instance.WaitBlitter();
        GfxDriver::Instance.SetFgColor(m_FgColor16);
        IPoint screenPos(m_ScreenPos);
        IRect rectScr(rect);
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
                GfxDriver::Instance.FillRect(clippedRect.left, clippedRect.top, clippedRect.right, clippedRect.bottom);
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
    GfxDriver::Instance.WaitBlitter();
    GfxDriver::Instance.SetCursor(m_ScreenPos.x + m_PenPosition.x, m_ScreenPos.y + m_PenPosition.y);
    GfxDriver::Instance.SetFont(m_Font->Get());
    GfxDriver::Instance.SetBgColor(m_BgColor16);
    GfxDriver::Instance.SetFgColor(m_FgColor16);
    m_PenPosition.x = GfxDriver::Instance.WriteString(string.c_str(), string.size(), maxWidth, flags) - m_ScreenPos.x;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerView::CopyRect(const Rect& srcRect, const Point& dstPos)
{
    if ( m_VisibleReg == nullptr ) {
        return;
    }

    IRect cISrcRect(srcRect);
    IPoint cDelta = IPoint( dstPos ) - cISrcRect.LeftTop();
    IRect cIDstRect = cISrcRect + cDelta;

    ClipRectList cBltList;
    Region       cDamage(*m_VisibleReg, cIDstRect, false);

    ENUMCLIPLIST( &m_VisibleReg->m_cRects, pcSrcClip )
    {
        // Clip to source rectangle
        IRect cSRect( cISrcRect & pcSrcClip->m_cBounds );

        if (!cSRect.IsValid()) {
            continue;
        }
        // Transform into destination space
        cSRect += cDelta;

        ENUMCLIPLIST( &m_VisibleReg->m_cRects, pcDstClip )
        {
            IRect cDRect = cSRect & pcDstClip->m_cBounds;

            if (!cDRect.IsValid()) {
                continue;
            }
            cDamage.Exclude( cDRect );
            ClipRect* pcClips = Region::AllocClipRect();
            pcClips->m_cBounds  = cDRect;
            pcClips->m_cMove    = cDelta;

            cBltList.AddRect( pcClips );
        }
    }

    int nCount = cBltList.GetCount();
    
    if (nCount == 0) {
        Invalidate( cIDstRect );
        UpdateIfNeeded( true );
        return;
    }

    IPoint cTopLeft( ConvertToRoot( Point( 0, 0 ) ) );

    ClipRect** apsClips = new ClipRect*[nCount];

    for ( int i = 0 ; i < nCount ; ++i )
    {
        apsClips[i] = cBltList.RemoveHead();
        assert( apsClips[i] != nullptr );
    }
    qsort( apsClips, nCount, sizeof( ClipRect* ), SortCmp );

    for ( int i = 0 ; i < nCount ; ++i )
    {
        ClipRect* pcClip = apsClips[i];
        pcClip->m_cBounds += cTopLeft; // Convert into screen space
        GfxDriver::Instance.BLT_MoveRect(pcClip->m_cBounds - pcClip->m_cMove, IPoint( pcClip->m_cBounds.left, pcClip->m_cBounds.top ));
        Region::FreeClipRect( pcClip );
    }
    delete[] apsClips;
    if (m_DamageReg != nullptr)
    {
        Region cReg( *m_DamageReg, cISrcRect, false );
        ENUMCLIPLIST( &cReg.m_cRects, pcDmgClip )
        {
            m_DamageReg->Include( (pcDmgClip->m_cBounds + cDelta)  & cIDstRect );
            if (m_ActiveDamageReg != nullptr) {
                m_ActiveDamageReg->Exclude((pcDmgClip->m_cBounds + cDelta)  & cIDstRect);
            }
        }
    }
    if (m_ActiveDamageReg != nullptr)
    {
        Region cReg(*m_ActiveDamageReg, cISrcRect, false);
        if (cReg.m_cRects.GetCount() > 0)
        {
            if ( m_DamageReg == nullptr ) {
                m_DamageReg = ptr_new<Region>();
            }
            ENUMCLIPLIST( &cReg.m_cRects, pcDmgClip )
            {
                m_ActiveDamageReg->Exclude((pcDmgClip->m_cBounds + cDelta) & cIDstRect);
                m_DamageReg->Include((pcDmgClip->m_cBounds + cDelta) & cIDstRect);
            }
        }
    }
    ENUMCLIPLIST( &cDamage.m_cRects, pcDstClip ) {
        Invalidate( pcDstClip->m_cBounds );
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
    IPoint oldOffset(m_ScrollOffset);
    m_ScrollOffset += offset;
    IPoint newOffset(m_ScrollOffset);
    
    if ( newOffset == oldOffset ) {
        return;
    }
    IPoint intOffset(newOffset - oldOffset);
    
    for (Ptr<ServerView> child : m_ChildrenList)
    {
        child->m_IFrame += intOffset;
        child->m_Frame  += offset;
    }
    if ( m_HideCount > 0 ) {
        return;
    }

    UpdateRegions();
    //SrvWindow::HandleMouseTransaction();
    
    if (m_FullReg == nullptr /*|| m_pcBitmap == nullptr*/ ) {
        return;
    }

    Rect         cBounds = GetBounds();
    IRect        cIBounds( cBounds );
    ClipRectList cBltList;
    Region       cDamage( *m_VisibleReg );

    ENUMCLIPLIST( &m_FullReg->m_cRects, pcSrcClip )
    {
        // Clip to source rectangle
        IRect cSRect = cIBounds & pcSrcClip->m_cBounds;

        // Transform into destination space
        if ( cSRect.IsValid() == false ) {
            continue;
        }
        cSRect += intOffset;

        ENUMCLIPLIST( &m_FullReg->m_cRects, pcDstClip )
        {
            IRect cDRect = cSRect & pcDstClip->m_cBounds;

            if ( cDRect.IsValid() == false ) {
                continue;
            }
            cDamage.Exclude( cDRect );

            ClipRect* pcClips = Region::AllocClipRect();
            pcClips->m_cBounds  = cDRect;
            pcClips->m_cMove    = intOffset;

            cBltList.AddRect( pcClips );
        }
    }
  
    int nCount = cBltList.GetCount();
    
    if ( nCount == 0 ) {
        Invalidate( cIBounds );
        UpdateIfNeeded( true );
        return;
    }
    IPoint cTopLeft( ConvertToRoot( Point( 0, 0 ) ) );

    ClipRect** apsClips = new ClipRect*[nCount];

    for ( int i = 0 ; i < nCount ; ++i )
    {
        apsClips[i] = cBltList.RemoveHead();
        assert( apsClips[i] != nullptr );
    }
    qsort( apsClips, nCount, sizeof( ClipRect* ), SortCmp );

    for ( int i = 0 ; i < nCount ; ++i )
    {
        ClipRect* pcClip = apsClips[i];

        pcClip->m_cBounds += cTopLeft; // Convert into screen space

        GfxDriver::Instance.BLT_MoveRect(pcClip->m_cBounds - pcClip->m_cMove, IPoint( pcClip->m_cBounds.left, pcClip->m_cBounds.top ));
        Region::FreeClipRect( pcClip );
    }
    delete[] apsClips;

    if (m_DamageReg != nullptr) {
        ENUMCLIPLIST( &m_DamageReg->m_cRects, pcDstClip ) {
            pcDstClip->m_cBounds += intOffset;
        }
    }

    if (m_ActiveDamageReg != nullptr) {
        ENUMCLIPLIST( &m_ActiveDamageReg->m_cRects, pcDstClip ) {
            pcDstClip->m_cBounds += intOffset;
        }
    }
    ENUMCLIPLIST( &cDamage.m_cRects, pcDstClip ) {
        Invalidate(pcDstClip->m_cBounds);
    }
    UpdateIfNeeded(true);
}

