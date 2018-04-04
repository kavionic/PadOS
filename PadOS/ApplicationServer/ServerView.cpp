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

ServerView::ServerView(const String& name) : ViewBase(name)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool ServerView::HandleMouseDown(MouseButton_e button, const Point& position)
{
    if (m_ClientHandle != -1 ) {
        ASHandleMouseDown::Sender::Emit(m_ClientPort, m_ClientHandle, button, position);
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
    if (m_ClientHandle != -1 ) {
        ASHandleMouseUp::Sender::Emit(m_ClientPort, m_ClientHandle, button, position);
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
        ASHandleMouseMove::Sender::Emit(m_ClientPort, m_ClientHandle, button, position);
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
        if (m_cIFrame == cIRect) {
            return;
        }
        m_DeltaMove += IPoint(cIRect.left, cIRect.top ) - IPoint( m_cIFrame.left, m_cIFrame.top);
        m_DeltaSize += IPoint(cIRect.Width(), cIRect.Height() ) - IPoint( m_cIFrame.Width(), m_cIFrame.Height());

        if (parent != nullptr)
        {
            parent->m_HasInvalidRegs = true;

            SetDirtyRegFlags();

            auto i = parent->GetChildRIterator(ptr_tmp_cast(this));
            if (i != parent->m_ChildrenList.rend())
            {
                for (++i; i != parent->m_ChildrenList.rend(); ++i)
                {
                    Ptr<ServerView> sibling = *i;
                    if (sibling->m_cIFrame.DoIntersect( m_cIFrame ) || sibling->m_cIFrame.DoIntersect( cIRect ))
                    {
                        sibling->MarkModified( m_cIFrame - sibling->m_cIFrame.LeftTop() );
                        sibling->MarkModified( cIRect - sibling->m_cIFrame.LeftTop() );
                    }
                }                
            }
        }            
    }
    m_cIFrame = cIRect;
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
            parent->m_HasInvalidRegs = true;

            SetDirtyRegFlags();
            auto i = parent->GetChildRIterator(ptr_tmp_cast(this));
            if (i != parent->m_ChildrenList.rend())
            {
                for (++i; i != parent->m_ChildrenList.rend(); ++i)
                {
                    Ptr<ServerView> sibling = *i;
                    if ( sibling->m_cIFrame.DoIntersect( m_cIFrame ) ) {
                        sibling->MarkModified( m_cIFrame - sibling->m_cIFrame.LeftTop() );
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
        if ( ((m_Flags & WID_FULL_UPDATE_ON_H_RESIZE) && m_DeltaSize.x != 0) || ((m_Flags & WID_FULL_UPDATE_ON_V_RESIZE) && m_DeltaSize.y != 0) )
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

            IPoint cChildOffset( IPoint( child->m_cIFrame.left, child->m_cIFrame.top ) + cTopLeft );
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

                cRect.left = cRect.right + int( parent->m_DeltaSize.x + parent->m_cIFrame.right -
                                                parent->m_cIFrame.left - m_cIFrame.right );

                if ( cRect.IsValid() ) {
                    Invalidate( cRect );
                }
            }
            if ( parent->m_DeltaSize.y < 0 ) {
                IRect cRect = cIBounds;

                cRect.top = cRect.bottom + int( parent->m_DeltaSize.y + parent->m_cIFrame.bottom -
                                                parent->m_cIFrame.top - m_cIFrame.bottom );

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
            m_FullReg = ptr_new<Region>( m_cIFrame );
        }
        else
        {
            assert(parent->m_FullReg != nullptr);
            if ( parent->m_FullReg == nullptr ) {
                m_FullReg = ptr_new<Region>( m_cIFrame );
            } else {
                m_FullReg = ptr_new<Region>( *parent->m_FullReg, m_cIFrame, true );
            }
            if ( m_ShapeConstrainReg != nullptr ) {
                m_FullReg->Intersect( *m_ShapeConstrainReg );
            }
            IPoint cLeftTop( m_cIFrame.LeftTop() );
            auto i = parent->GetChildIterator(ptr_tmp_cast(this));
            if (i != parent->m_ChildrenList.end())
            {
                for (++i; i != parent->m_ChildrenList.end(); ++i)
                {
                    Ptr<ServerView> sibling = *i;
                    if ( sibling->m_HideCount == 0 )
                    {
                        if ( sibling->m_cIFrame.DoIntersect( m_cIFrame ) )
                        {
                            if ( sibling->m_ShapeConstrainReg == nullptr ) {
                                m_FullReg->Exclude( sibling->m_cIFrame - cLeftTop );
                            } else {
                                m_FullReg->Exclude( *sibling->m_ShapeConstrainReg, sibling->m_cIFrame.LeftTop() - cLeftTop );
                            }
                        }
                    }
                }
            }                
        }
        m_FullReg->Optimize();
        m_VisibleReg = ptr_new<Region>(*m_FullReg);

        if ( (m_Flags & WID_DRAW_ON_CHILDREN) == 0 )
        {
            bool bRegModified = false;
            for (Ptr<ServerView> child : m_ChildrenList)
            {
                // Remove children from child region
                if ( child->m_HideCount == 0 && (child->m_Flags & WID_TRANSPARENT) == 0 )
                {
                    if ( child->m_ShapeConstrainReg == nullptr ) {
                        m_VisibleReg->Exclude(child->m_cIFrame);
                    } else {
                        m_VisibleReg->Exclude(*child->m_ShapeConstrainReg, child->m_cIFrame.LeftTop());
                    }
                    bRegModified = true;
                }
            }
            if ( bRegModified ) {
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
    ASPaintView::Sender::Emit(MessagePort(m_ClientPort), m_ClientHandle, updateRect);
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
            child->MarkModified( rect - child->m_cIFrame.LeftTop() );
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
        parent->m_HasInvalidRegs = true;
        SetDirtyRegFlags();
        for (Ptr<ServerView> sibling : parent->m_ChildrenList)
        {
            if ( sibling->m_cIFrame.DoIntersect( m_cIFrame ) ) {
                sibling->MarkModified( m_cIFrame - sibling->m_cIFrame.LeftTop() );
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

void ServerView::ScrollRect(const Rect& srcRect, const Point& dstPos)
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
        //ClipRect* pcDmgClip;
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

void ServerView::ScrollBy(const Point& cOffset)
{
    Ptr<ServerView> parent = m_Parent.Lock();
    if ( parent == nullptr ) {
        return;
    }
    IPoint cOldOffset = m_cIScrollOffset;
    m_ScrollOffset += cOffset;
    m_cIScrollOffset = static_cast<IPoint>(m_ScrollOffset);

    if ( m_cIScrollOffset == cOldOffset ) {
        return;
    }
    
    IPoint cIOffset = m_cIScrollOffset - cOldOffset;

    for (Ptr<ServerView> child : m_ChildrenList)
    {
        child->m_cIFrame += cIOffset;
        child->m_Frame  += static_cast<Point>(cIOffset);
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
        cSRect += cIOffset;

        ENUMCLIPLIST( &m_FullReg->m_cRects, pcDstClip )
        {
            IRect cDRect = cSRect & pcDstClip->m_cBounds;

            if ( cDRect.IsValid() == false ) {
                continue;
            }
            cDamage.Exclude( cDRect );

            ClipRect* pcClips = Region::AllocClipRect();
            pcClips->m_cBounds  = cDRect;
            pcClips->m_cMove    = cIOffset;

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
            pcDstClip->m_cBounds += cIOffset;
        }
    }

    if (m_ActiveDamageReg != nullptr) {
        ENUMCLIPLIST( &m_ActiveDamageReg->m_cRects, pcDstClip ) {
            pcDstClip->m_cBounds += cIOffset;
        }
    }
    ENUMCLIPLIST( &cDamage.m_cRects, pcDstClip ) {
        Invalidate(pcDstClip->m_cBounds);
    }
    UpdateIfNeeded(true);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerView::Sync()
{
    ASViewSyncReply::Sender::Emit(MessagePort(m_ClientPort), m_ClientHandle);    
}
