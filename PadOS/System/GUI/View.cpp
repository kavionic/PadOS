// This file is part of PadOS.
//
// Copyright (C) 2014-2018 Kurt Skauen <http://kavionic.com/>
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
// Created: 30.01.2014 23:46:08

#include "sam.h"

#include "View.h"

#include <algorithm>
#include <stddef.h>
#include <stdio.h>
#include "GUI.h"

using namespace kernel;

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

View::View()
{    
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

View::~View()
{
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void View::AddChild(Ptr<View> child)
{
    LinkChild(child, true);
    child->Added(m_HideCount, m_Level + 1);	// Alloc clipping regions
    UpdateRegions();
    UpdateIfNeeded(false);
    if (m_IsAttachedToScreen)
    {
        child->HandlePostAttachedToScreen();
    }

}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void View::RemoveChild(Ptr<View> child)
{
    UnlinkChild(child);
    UpdateRegions();
}

void View::Added(int hideCount, int level)
{
    m_HideCount += hideCount;
    m_Level = level;

    for ( Ptr<View> child = m_TopChild ; child != nullptr ; child = child->m_LowerSibling ) {
	child->Added(hideCount, level + 1);
    }
}

void View::DeleteRegions()
{
    assert( m_PrevVisibleReg == nullptr );
    assert( m_PrevFullReg == nullptr );

    delete m_pcVisibleReg;
    delete m_pcFullReg;
    delete m_pcDamageReg;
    delete m_pcActiveDamageReg;
    delete m_pcDrawReg;
  
    m_pcVisibleReg     	= nullptr;
    m_pcFullReg	     	= nullptr;
    m_pcDrawReg	     	= nullptr;
    m_pcDamageReg	= nullptr;
    m_pcActiveDamageReg = nullptr;
    m_pcDrawReg	     	= nullptr;
  
    for ( Ptr<View> child = m_TopChild ; child != nullptr ; child = child->m_LowerSibling ) {
	child->DeleteRegions();
    }
}

void View::UpdateIfNeeded(bool force)
{
    if ( m_HideCount > 0 || !m_IsAttachedToScreen) {
	return;
    }
//    if ( m_pcWindow != nullptr && ((m_pcWindow->GetDesktopMask() & (1 << get_active_desktop())) == 0 ||
//				m_pcWindow->HasPendingSizeEvents( this )) ) {
//	return;
//    }
  
    if ( m_pcDamageReg != nullptr ) {
	if ( m_pcActiveDamageReg == nullptr ) {
	    m_pcActiveDamageReg = m_pcDamageReg;
	    m_pcDamageReg = nullptr;
	    m_pcActiveDamageReg->Optimize();
	    Paint( static_cast<Rect>(m_pcActiveDamageReg->GetBounds()), true );
	}
    }
    for ( Ptr<View> child = m_BottomChild ; child != nullptr ; child = child->m_HigherSibling ) {
        child->UpdateIfNeeded(force);
    }
}

void View::MarkModified(const IRect& rect)
{
    if ( GetBounds().DoIntersect( rect ) )
    {
	m_bHasInvalidRegs = true;
    
        for ( Ptr<View> child = m_BottomChild ; child != nullptr ; child = child->m_HigherSibling ) {
	    child->MarkModified( rect - child->m_cIFrame.LeftTop() );
	}
    }
}

void View::SetDirtyRegFlags()
{
    m_bHasInvalidRegs = true;
    
    for ( Ptr<View> child = m_BottomChild ; child != nullptr ; child = child->m_HigherSibling ) {
	child->SetDirtyRegFlags();
    }
}

int SortCmp(const void* pNode1, const void* pNode2)
{
    ClipRect* pcClip1 = *((ClipRect**)pNode1);
    ClipRect* pcClip2 = *((ClipRect**)pNode2);


    if ( pcClip1->m_cBounds.left > pcClip2->m_cBounds.right && pcClip1->m_cBounds.right < pcClip2->m_cBounds.left ) {
	if ( pcClip1->m_cMove.x < 0 ) {
	    return( pcClip1->m_cBounds.left - pcClip2->m_cBounds.left );
	} else {
	    return( pcClip2->m_cBounds.left - pcClip1->m_cBounds.left );
	}
    } else {
	if ( pcClip1->m_cMove.y < 0 ) {
	    return( pcClip1->m_cBounds.top - pcClip2->m_cBounds.top );
	} else {
	    return( pcClip2->m_cBounds.top - pcClip1->m_cBounds.top );
	}
    }
}

void View::ScrollBy(const Point& cOffset)
{
    Ptr<View> parent = m_Parent.Lock();
    if ( parent == nullptr ) {
	return;
    }
    IPoint cOldOffset = m_cIScrollOffset;
    m_cScrollOffset += cOffset;
    m_cIScrollOffset = static_cast<IPoint>(m_cScrollOffset);

    if ( m_cIScrollOffset == cOldOffset ) {
	return;
    }
//    Render();
//    return;    
    IPoint cIOffset = m_cIScrollOffset - cOldOffset;

    for ( Ptr<View> child = m_BottomChild ; child != nullptr ; child = child->m_HigherSibling ) {
	child->m_cIFrame += cIOffset;
	child->m_Frame  += static_cast<Point>(cIOffset);
    }
    if ( m_HideCount > 0 ) {
	return;
    }

    UpdateRegions();
    //SrvWindow::HandleMouseTransaction();
    
    if (m_pcFullReg == nullptr || !m_IsAttachedToScreen /*|| m_pcBitmap == nullptr*/ ) {
	return;
    }

    Rect	 cBounds = GetBounds();
    IRect	 cIBounds( cBounds );
    ClipRectList cBltList;
    Region	 cDamage( *m_pcVisibleReg );

    ENUMCLIPLIST( &m_pcFullReg->m_cRects, pcSrcClip )
    {
	  // Clip to source rectangle
	IRect cSRect	= cIBounds & pcSrcClip->m_cBounds;

	// Transform into destination space
	if ( cSRect.IsValid() == false ) {
	    continue;
	}
	cSRect += cIOffset;

	ENUMCLIPLIST( &m_pcFullReg->m_cRects, pcDstClip )
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
	
	pcClip->m_cBounds += cTopLeft;	// Convert into screen space
	
	GfxDriver::Instance.BLT_MoveRect(pcClip->m_cBounds - pcClip->m_cMove, IPoint( pcClip->m_cBounds.left, pcClip->m_cBounds.top ));
	Region::FreeClipRect( pcClip );
    }
    delete[] apsClips;

    if ( m_pcDamageReg != nullptr ) {
	ENUMCLIPLIST( &m_pcDamageReg->m_cRects, pcDstClip ) {
	    pcDstClip->m_cBounds += cIOffset;
	}
    }

    if ( m_pcActiveDamageReg != nullptr ) {
	ENUMCLIPLIST( &m_pcActiveDamageReg->m_cRects, pcDstClip ) {
	    pcDstClip->m_cBounds += cIOffset;
	}
    }
    ENUMCLIPLIST( &cDamage.m_cRects, pcDstClip ) {
	Invalidate( pcDstClip->m_cBounds );
    }
    UpdateIfNeeded( true );
}

void View::SetFrame(const Rect& cRect)
{
    m_Frame = cRect;
    UpdateScreenPos();
    IRect cIRect( cRect );
    
    Ptr<View> parent = m_Parent.Lock();
    
    if ( m_HideCount == 0 )
    {
	if ( m_cIFrame == cIRect ) {
	    return;
	}
	m_cDeltaMove += IPoint( cIRect.left, cIRect.top ) - IPoint( m_cIFrame.left, m_cIFrame.top );
	m_cDeltaSize += IPoint( cIRect.Width(), cIRect.Height() ) - IPoint( m_cIFrame.Width(), m_cIFrame.Height() );

	if ( parent != nullptr ) {
	    parent->m_bHasInvalidRegs = true;
	}

	SetDirtyRegFlags();

	for (Ptr<View> sibling = m_LowerSibling ; sibling != nullptr ; sibling = sibling->m_LowerSibling ) {
	    if ( sibling->m_cIFrame.DoIntersect( m_cIFrame ) || sibling->m_cIFrame.DoIntersect( cIRect ) ) {
		sibling->MarkModified( m_cIFrame - sibling->m_cIFrame.LeftTop() );
		sibling->MarkModified( cIRect - sibling->m_cIFrame.LeftTop() );
	    }
	}
    }
    m_cIFrame = cIRect;
    if ( parent == nullptr ) {
	Invalidate();
    }
}

void View::SetDrawRegion(Region* pcReg)
{
    delete m_pcDrawConstrainReg;
    m_pcDrawConstrainReg = pcReg;

    if ( m_HideCount == 0 ) {
	m_bHasInvalidRegs = true;
    }
    delete m_pcDrawReg;
    m_pcDrawReg = nullptr;
}

void View::SetShapeRegion(Region* pcReg)
{
    delete m_pcShapeConstrainReg;
    m_pcShapeConstrainReg = pcReg;

    if ( m_HideCount == 0 )
    {
        Ptr<View> parent = m_Parent.Lock();
	if ( parent != nullptr ) {
	    parent->m_bHasInvalidRegs = true;
	}
	SetDirtyRegFlags();
	for (Ptr<View> sibling = m_LowerSibling ; sibling != nullptr ; sibling = sibling->m_LowerSibling ) {
	    if ( sibling->m_cIFrame.DoIntersect( m_cIFrame ) ) {
		sibling->MarkModified( m_cIFrame - sibling->m_cIFrame.LeftTop() );
	    }
	}
    }
}

void View::Invalidate( const IRect& cRect )
{
    if ( m_HideCount == 0 )
    {
	if ( m_pcDamageReg == nullptr ) {
	    m_pcDamageReg = new Region( cRect );
	} else {
	    m_pcDamageReg->Include( cRect );
	}
    }
}

void View::Invalidate(bool bReqursive)
{
    if ( m_HideCount == 0 )
    {
	delete m_pcDamageReg;
	m_pcDamageReg = new Region( static_cast<IRect>(GetBounds()) );
	if ( bReqursive ) {
            for ( Ptr<View> child = m_BottomChild ; child != nullptr ; child = child->m_HigherSibling ) {
		child->Invalidate(true);
	    }
	}
    }
}

void View::InvalidateNewAreas()
{
    Region* pcRegion;

    if ( m_HideCount > 0 ) {
	return;
    }
    if (!m_IsAttachedToScreen /*m_pcWindow != nullptr && (m_pcWindow->GetDesktopMask() & (1 << get_active_desktop())) == 0*/ ) {
	return;
    }
  
    if ( m_bHasInvalidRegs )
    {
	if ( ((m_Flags & WID_FULL_UPDATE_ON_H_RESIZE) && m_cDeltaSize.x != 0) || ((m_Flags & WID_FULL_UPDATE_ON_V_RESIZE) && m_cDeltaSize.y != 0) )
        {
	    Invalidate(false);
	}
        else
        {
	    if ( m_pcVisibleReg != nullptr )
            {
		pcRegion = new Region( *m_pcVisibleReg );
    
		if ( pcRegion != nullptr )
                {
		    if ( m_PrevVisibleReg != nullptr ) {
			pcRegion->Exclude( *m_PrevVisibleReg );
		    }
		    if ( m_pcDamageReg == nullptr )
                    {
			if ( pcRegion->IsEmpty() == false ) {
			    m_pcDamageReg = pcRegion;
			} else {
			    delete pcRegion;
			}
		    }
                    else
                    {
			ENUMCLIPLIST( &pcRegion->m_cRects, pcClip ) {
			    Invalidate( pcClip->m_cBounds );
			}
			delete pcRegion;
		    }
		}
	    }
	}
	delete m_PrevVisibleReg;
	m_PrevVisibleReg = nullptr;
	
	m_cDeltaSize = IPoint( 0, 0 );
	m_cDeltaMove = IPoint( 0, 0 );
    }
  
    for ( Ptr<View> child = m_BottomChild ; child != nullptr ; child = child->m_HigherSibling ) {
	child->InvalidateNewAreas();
    }
}

void View::MoveChilds()
{
    if ( m_HideCount > 0 || !m_IsAttachedToScreen /*|| nullptr == m_pcBitmap */) {
	return;
    }
    if ( !m_IsAttachedToScreen /*m_pcWindow != nullptr && (m_pcWindow->GetDesktopMask() & (1 << get_active_desktop())) == 0*/ ) {
	return;
    }
  
    if ( m_bHasInvalidRegs )
    {
	Rect cBounds = GetBounds();
	IRect cIBounds( cBounds );
	
        for ( Ptr<View> child = m_BottomChild ; child != nullptr ; child = child->m_HigherSibling )
	{
	    if ( child->m_cDeltaMove.x == 0.0f && child->m_cDeltaMove.y == 0.0f ) {
		continue;
	    }
	    if ( child->m_pcFullReg == nullptr || child->m_PrevFullReg == nullptr ) {
		continue;
	    }

	    Region* pcRegion = new Region( *child->m_PrevFullReg );
	    pcRegion->Intersect( *child->m_pcFullReg );

	    int nCount = 0;
    
	    IPoint cTopLeft( ConvertToRoot( Point( 0, 0 ) ) );

	    IPoint cChildOffset( IPoint( child->m_cIFrame.left, child->m_cIFrame.top ) + cTopLeft );
	    IPoint cChildMove( child->m_cDeltaMove );
	    ENUMCLIPLIST( &pcRegion->m_cRects, pcClip )
            {
		  // Transform into parents coordinate system
		pcClip->m_cBounds += cChildOffset;
		pcClip->m_cMove    = cChildMove;
		nCount++;
		assert( pcClip->m_cBounds.IsValid() );
	    }
	    if ( nCount == 0 ) {
		delete pcRegion;
		continue;
	    }

	    ClipRect** apsClips = new ClipRect*[nCount];

	    for ( int i = 0 ; i < nCount ; ++i ) {
		apsClips[i] = pcRegion->m_cRects.RemoveHead();
		assert( apsClips[i] != nullptr );
	    }
	    qsort( apsClips, nCount, sizeof( ClipRect* ), SortCmp );

	    for ( int i = 0 ; i < nCount ; ++i ) {
		ClipRect* pcClip = apsClips[i];
	
		GfxDriver::Instance.BLT_MoveRect(pcClip->m_cBounds - pcClip->m_cMove, IPoint( pcClip->m_cBounds.left, pcClip->m_cBounds.top ));
		m_pcVisibleReg->FreeClipRect( pcClip );
	    }
	    delete[] apsClips;
	    delete pcRegion;
	}
	  /*	Since the parent window is shrinked before the childs is moved
	   *	we may need to redraw the right and bottom edges.
	   */
        Ptr<View> parent = m_Parent.Lock();
	if ( parent != nullptr && (m_cDeltaMove.x != 0.0f || m_cDeltaMove.y != 0.0f) )
        {
	    if ( parent->m_cDeltaSize.x < 0 ) {
		IRect	cRect = cIBounds;

		cRect.left = cRect.right + int( parent->m_cDeltaSize.x + parent->m_cIFrame.right -
						parent->m_cIFrame.left - m_cIFrame.right );

		if ( cRect.IsValid() ) {
		    Invalidate( cRect );
		}
	    }
	    if ( parent->m_cDeltaSize.y < 0 ) {
		IRect	cRect = cIBounds;

		cRect.top = cRect.bottom + int( parent->m_cDeltaSize.y + parent->m_cIFrame.bottom -
						parent->m_cIFrame.top - m_cIFrame.bottom );

		if ( cRect.IsValid() ) {
		    Invalidate( cRect );
		}
	    }
	}
	delete m_PrevFullReg;
	m_PrevFullReg = nullptr;
    }
    for ( Ptr<View> child = m_BottomChild ; child != nullptr ; child = child->m_HigherSibling ) {
	child->MoveChilds();
    }
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
//	Stores the previous visible region in m_PrevVisibleReg and then
//	rebuilds m_pcVisibleReg, starting with whatever is left of our parent
//	and then removing areas covered by siblings.
// NOTE:
//	Areas covered by childrens are not removed.
// SEE ALSO:
//----------------------------------------------------------------------------

void View::SwapRegions( bool bForce )
{
    if ( bForce ) {
	m_bHasInvalidRegs = true;
    }
  
    for ( Ptr<View> child = m_BottomChild ; child != nullptr ; child = child->m_HigherSibling ) {
	child->SwapRegions( bForce );
    }
}


void View::RebuildRegion( bool bForce )
{
    if (!m_IsAttachedToScreen /*m_pcWindow != nullptr && (m_pcWindow->GetDesktopMask() & (1 << get_active_desktop())) == 0*/ ) {
	return;
    }

    if ( m_HideCount > 0 )
    {
	if ( m_pcVisibleReg != nullptr ) {
	    DeleteRegions();
	}
	return;
    }
  
    if ( bForce ) {
	m_bHasInvalidRegs = true;
    }
  
    if ( m_bHasInvalidRegs )
    {
	delete m_pcDrawReg;
	m_pcDrawReg = nullptr;

	assert( m_PrevVisibleReg == nullptr );
	assert( m_PrevFullReg == nullptr );
    
	m_PrevVisibleReg = m_pcVisibleReg;
	m_PrevFullReg    = m_pcFullReg;

        Ptr<View> parent = m_Parent.Lock();
	if ( parent == nullptr )
        {
	    m_pcFullReg = new Region( m_cIFrame );
	}
        else
        {
	    assert( parent->m_pcFullReg != nullptr );
	    if ( parent->m_pcFullReg == nullptr ) {
		m_pcFullReg = new Region( m_cIFrame );
	    } else {
		m_pcFullReg = new Region( *parent->m_pcFullReg, m_cIFrame, true );
	    }
	    if ( m_pcShapeConstrainReg != nullptr ) {
		m_pcFullReg->Intersect( *m_pcShapeConstrainReg );
	    }
	}
//    if ( m_nLevel == 1 ) {
	IPoint cLeftTop( m_cIFrame.LeftTop() );
	for (Ptr<View> sibling = m_HigherSibling ; sibling != nullptr ; sibling = sibling->m_HigherSibling )
        {
	    if ( sibling->m_HideCount == 0 )
            {
		if ( sibling->m_cIFrame.DoIntersect( m_cIFrame ) )
                {
		    if ( sibling->m_pcShapeConstrainReg == nullptr ) {
			m_pcFullReg->Exclude( sibling->m_cIFrame - cLeftTop );
		    } else {
			m_pcFullReg->Exclude( *sibling->m_pcShapeConstrainReg, sibling->m_cIFrame.LeftTop() - cLeftTop );
		    }
		}
	    }
	}
//    }
	m_pcFullReg->Optimize();
	m_pcVisibleReg = new Region( *m_pcFullReg );

	if ( (m_Flags & WID_DRAW_ON_CHILDREN) == 0 )
        {
	    bool bRegModified = false;
            for ( Ptr<View> child = m_BottomChild ; child != nullptr ; child = child->m_HigherSibling )
            {
		  /***	Remove children from child region	***/
		if ( child->m_HideCount == 0 && (child->m_Flags & WID_TRANSPARENT) == 0 )
                {
		    if ( child->m_pcShapeConstrainReg == nullptr ) {
			m_pcVisibleReg->Exclude(child->m_cIFrame);
		    } else {
			m_pcVisibleReg->Exclude( *child->m_pcShapeConstrainReg, child->m_cIFrame.LeftTop() );
		    }
		    bRegModified = true;
		}
	    }
	    if ( bRegModified ) {
		m_pcVisibleReg->Optimize();
	    }
	}
    }
    for ( Ptr<View> child = m_BottomChild ; child != nullptr ; child = child->m_HigherSibling ) {
        child->RebuildRegion(bForce);
    }
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void View::ClearDirtyRegFlags()
{
    m_bHasInvalidRegs = false;
    for ( Ptr<View> child = m_BottomChild ; child != nullptr ; child = child->m_HigherSibling ) {
        child->ClearDirtyRegFlags();
    }
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void View::UpdateRegions(bool bForce, bool bRoot)
{
    RebuildRegion( bForce );
    MoveChilds();
    InvalidateNewAreas();

    if ( m_bHasInvalidRegs && m_Parent.Lock() == nullptr && m_IsAttachedToScreen /*m_pcBitmap != nullptr*/ && m_pcDamageReg != nullptr )
    {
//	if ( m_pcBitmap == g_pcScreenBitmap )
        {
	    uint16_t color = LCD_RGB( 0x00, 0x60, 0x6b);
	    IPoint    cTopLeft( ConvertToRoot( Point( 0, 0 ) ) );

//	    Region cTmpReg( static_cast<IRect>(GetBounds()) );

	    Region cDrawReg( *m_pcVisibleReg );
	    cDrawReg.Intersect( *m_pcDamageReg );
//	    cTmpReg.Exclude( *m_pcDamageReg );
//	    cDrawReg.Exclude( cTmpReg );
	
            GfxDriver::Instance.SetFgColor(color);
	    ENUMCLIPLIST( &cDrawReg.m_cRects, pcClip ) {
		GfxDriver::Instance.FillRect(pcClip->m_cBounds + cTopLeft);
	    }
	}
	delete m_pcDamageReg;
	m_pcDamageReg = nullptr;
    }
    UpdateIfNeeded( false );
    ClearDirtyRegFlags();
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Region* View::GetRegion()
{
    if ( m_HideCount > 0 ) {
	return nullptr;
    }
    if ( m_bIsUpdating && m_pcActiveDamageReg == nullptr ) {
	return nullptr;
    }

    if ( !m_bIsUpdating )
    {
	if ( m_pcDrawConstrainReg == nullptr ) {
	    return m_pcVisibleReg;
	} else {
	    if ( m_pcDrawReg == nullptr ) {
		m_pcDrawReg = new Region( *m_pcVisibleReg );
		m_pcDrawReg->Intersect( *m_pcDrawConstrainReg );
	    }
	}
    }
    else if ( m_pcDrawReg == nullptr && m_pcVisibleReg != nullptr )
    {
	m_pcDrawReg = new Region( *m_pcVisibleReg );

	assert( m_pcActiveDamageReg != nullptr );
	m_pcDrawReg->Intersect( *m_pcActiveDamageReg );
	if ( m_pcDrawConstrainReg != nullptr ) {
	    m_pcDrawReg->Intersect( *m_pcDrawConstrainReg );
	}
	m_pcDrawReg->Optimize();
    }
    assert( m_pcDrawReg != nullptr);
    return m_pcDrawReg;
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void View::PutRegion( Region* pcReg )
{
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void View::BeginUpdate()
{
    if ( m_pcVisibleReg != nullptr ) {
	m_bIsUpdating = true;
    }
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void View::EndUpdate( void )
{
    delete m_pcActiveDamageReg;
    m_pcActiveDamageReg = nullptr;
    delete m_pcDrawReg;
    m_pcDrawReg = nullptr;
    m_bIsUpdating	= false;
  
    if ( m_pcDamageReg != nullptr ) {
	m_pcActiveDamageReg = m_pcDamageReg;
	m_pcDamageReg = nullptr;
	Paint( static_cast<Rect>(m_pcActiveDamageReg->GetBounds()), true );
    }
}

void View::Paint( const IRect& cUpdateRect, bool bUpdate )
{
    if ( m_HideCount > 0 || m_bIsUpdating == true /*|| nullptr == m_pUserObject*/ ) {
	return;
    }
    BeginUpdate();
    Render();
    EndUpdate();
/*    Messenger*	pcTarget = m_pcWindow->GetAppTarget();

    if ( nullptr != pcTarget ) {
	Message cMsg( M_PAINT );

	cMsg.AddPointer( "_widget", GetUserObject() );
	cMsg.AddRect( "_rect", cUpdateRect - m_cIScrollOffset );

	if ( pcTarget->SendMessage( &cMsg ) < 0 ) {
	    dbprintf( "Layer::Paint() failed to send M_PAINT message to %s\n", m_cName.c_str() );
	}
    }*/
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void View::Move(const Point& delta)
{
    SetFrame(m_Frame + delta);
//    m_Frame += delta;    
//    OffsetScreenPos(delta);
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////
/*
void View::SetFrame(const Rect& frame)
{
//    Point delta = frame.LeftTop() - m_Frame.LeftTop();

    m_Frame = frame;
    
    UpdateScreenPos();
//    OffsetScreenPos(delta);
}*/

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

const Rect& View::GetFrame() const
{
    return m_Frame;
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void View::Show(bool doShow)
{
//    if ( m_pcParent == nullptr || m_pcWindow == nullptr ) {
//	dbprintf( "Error: Layer::Show() attempt to hide root layer\n" );
//	return;
//    }
    if ( doShow ) {
	m_HideCount--;
    } else {
	m_HideCount++;
    }

    Ptr<View> parent = m_Parent.Lock();
    if (parent != nullptr)
    {
        parent->m_bHasInvalidRegs = true;
        SetDirtyRegFlags();
        for ( Ptr<View> sibling = parent->m_BottomChild ; sibling != nullptr ; sibling = sibling->m_HigherSibling ) {
	    if ( sibling->m_cIFrame.DoIntersect( m_cIFrame ) ) {
	        sibling->MarkModified( m_cIFrame - sibling->m_cIFrame.LeftTop() );
	    }
        }
    }
    for (Ptr<View> child = m_TopChild ; child != nullptr ; child = child->m_LowerSibling ) {
	child->Show( doShow );
    }
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

/*void View::SetCursor(float x, float y)
{
    GfxDriver::Instance.SetCursor(m_ScreenPos.x + x, m_ScreenPos.y + y);
}*/

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void View::DrawBevelBox(Rect frame, bool raised)
{
    uint16_t colorDark  = GfxDriver::MakeColor(0xa0,0xa0,0xa0);
    uint16_t colorLight = GfxDriver::MakeColor(0xe0,0xe0,0xe0);
    if ( !raised ) std::swap(colorDark, colorLight);

    SetFgColor(colorLight);
    DrawLine(frame.left, frame.top, frame.right, frame.top);
    
    SetFgColor(colorDark);
    DrawLine(frame.right, frame.top + 1, frame.right, frame.bottom);
    DrawLine(frame.right - 1, frame.bottom, frame.left - 1, frame.bottom);
    
    SetFgColor(colorLight);
    DrawLine(frame.left, frame.bottom, frame.left, frame.top);

    colorDark  = GfxDriver::MakeColor(0xc0,0xc0,0xc0);
    colorLight = GfxDriver::MakeColor(0xff,0xff,0xff);
    if ( !raised ) std::swap(colorDark, colorLight);
    
    frame.Resize(1.0f, 1.0f, -1.0f, -1.0f);
    
    SetFgColor(colorLight);
    DrawLine(frame.left, frame.top, frame.right, frame.top);

    SetFgColor(colorDark);
    DrawLine(frame.right, frame.top + 1, frame.right, frame.bottom);
    DrawLine(frame.right - 1, frame.bottom, frame.left - 1, frame.bottom);

    SetFgColor(colorLight);
    DrawLine(frame.left, frame.bottom, frame.left, frame.top);

    frame.Resize(1.0f, 1.0f, -1.0f, -1.0f);
    FillRect(frame, (raised) ? GfxDriver::MakeColor(0xd0,0xd0,0xd0) : GfxDriver::MakeColor(0xf0,0xf0,0xf0) );
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void View::DrawString(const char* string, uint32_t strLength, float maxWidth, uint8_t flags)
{
    GfxDriver::Instance.WriteString(string, strLength, maxWidth, flags);
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////
/*
void View::Invalidate()
{
    if (m_IsAttachedToScreen)
    {
        Render();
        for (Ptr<View> child = m_BottomChild; child != nullptr; child = child->m_HigherSibling)
        {
            child->Invalidate();
        }
    }
}
*/
///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

bool View::OnMouseDown(MouseButton_e button, const Point& position)
{
    if (!VFMouseDown.Empty()) {
        return VFMouseDown(button, position);
    } else {
        return false;
    }        
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

bool View::OnMouseUp(MouseButton_e button, const Point& position)
{
    if (!VFMouseUp.Empty()) {
        return VFMouseUp(button, position);
    } else {
        return false;
    }
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

bool View::OnMouseMove(MouseButton_e button, const Point& position)
{
    if (!VFMouseMoved.Empty()) {
        return VFMouseMoved(position);
    } else {
        return false;
    }        
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void View::DrawLine(const Point& toPoint)
{
    DrawLine(m_PenPosition, toPoint);
    m_PenPosition = toPoint;    
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void View::DrawLine(const Point& fromPnt, const Point& toPnt )
{
    const Region* region = GetRegion();
    if (region != nullptr)
    {
        IPoint screenPos(m_ScreenPos);
        IPoint fromPntScr(fromPnt + m_cScrollOffset);
        IPoint toPntScr(toPnt + m_cScrollOffset);
        fromPntScr += screenPos;
        toPntScr   += screenPos;
      
        IRect screenFrame = gui.GetScreenIFrame();

        if (!Region::ClipLine(screenFrame, &fromPntScr.x, &fromPntScr.y, &toPntScr.x, &toPntScr.y)) return;

        if (fromPntScr.x > toPntScr.x)
        {
            std::swap(toPntScr, fromPntScr);
        }
        
        GfxDriver::Instance.WaitBlitter();
        GfxDriver::Instance.SetFgColor(m_FgColor);
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
///
///////////////////////////////////////////////////////////////////////////////

void View::FillCircle(const Point& position, float radius )
{
    //DEBUG_DISABLE_IRQ();
    if (int(position.y + m_ScreenPos.y + radius) >= 510) return; // Broken clipping past that.
    
    const Region* region = GetRegion();
    if (region != nullptr)
    {
        IPoint screenPos(m_ScreenPos);
        IPoint positionScr(position + m_cScrollOffset);
        int    radiusRounded = int(radius + 0.5f);
        
        positionScr += screenPos;
        
        GfxDriver::Instance.WaitBlitter();
        GfxDriver::Instance.SetFgColor(m_FgColor);
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
///
///////////////////////////////////////////////////////////////////////////////

void View::ScrollRect(const Rect& srcRect, const Point& dstPos)
{
    if ( m_pcVisibleReg == nullptr ) {
	return;
    }

    IRect cISrcRect(srcRect);
    IPoint cDelta = IPoint( dstPos ) - cISrcRect.LeftTop();
    IRect cIDstRect = cISrcRect + cDelta;

//    ClipRect*	 pcSrcClip;
//    ClipRect*	 pcDstClip;
    ClipRectList cBltList;
    Region	 cDamage(*m_pcVisibleReg, cIDstRect, false);

    ENUMCLIPLIST( &m_pcVisibleReg->m_cRects, pcSrcClip )
    {
	  // Clip to source rectangle
	IRect	cSRect( cISrcRect & pcSrcClip->m_cBounds );

	if (!cSRect.IsValid()) {
	    continue;
	}
	  // Transform into destination space
	cSRect += cDelta;

	ENUMCLIPLIST( &m_pcVisibleReg->m_cRects, pcDstClip )
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
	pcClip->m_cBounds += cTopLeft;	// Convert into screen space
	GfxDriver::Instance.BLT_MoveRect(pcClip->m_cBounds - pcClip->m_cMove, IPoint( pcClip->m_cBounds.left, pcClip->m_cBounds.top ));
	Region::FreeClipRect( pcClip );
    }
    delete[] apsClips;
    if (m_pcDamageReg != nullptr)
    {
	//ClipRect* pcDmgClip;
	Region cReg( *m_pcDamageReg, cISrcRect, false );
	ENUMCLIPLIST( &cReg.m_cRects, pcDmgClip )
        {
	    m_pcDamageReg->Include( (pcDmgClip->m_cBounds + cDelta)  & cIDstRect );
	    if ( m_pcActiveDamageReg != nullptr ) {
		m_pcActiveDamageReg->Exclude( (pcDmgClip->m_cBounds + cDelta)  & cIDstRect );
	    }
	}
    }
    if (m_pcActiveDamageReg != nullptr)
    {
//	ClipRect* pcDmgClip;
	Region cReg( *m_pcActiveDamageReg, cISrcRect, false );
	if (cReg.m_cRects.GetCount() > 0)
        {
	    if ( m_pcDamageReg == nullptr ) {
		m_pcDamageReg = new Region();
	    }
	    ENUMCLIPLIST( &cReg.m_cRects, pcDmgClip )
            {
		m_pcActiveDamageReg->Exclude( (pcDmgClip->m_cBounds + cDelta) & cIDstRect );
		m_pcDamageReg->Include( (pcDmgClip->m_cBounds + cDelta) & cIDstRect );
	    }
	}
    }
    ENUMCLIPLIST( &cDamage.m_cRects, pcDstClip ) {
	Invalidate( pcDstClip->m_cBounds );
    }
    if ( m_pcDamageReg != nullptr ) {
	m_pcDamageReg->Optimize();
    }
    if ( m_pcActiveDamageReg != nullptr ) {
	m_pcActiveDamageReg->Optimize();
    }
    UpdateIfNeeded(true);
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void View::FillRect(const Rect& rect)
{
    const Region* region = GetRegion();
    if (region != nullptr)
    {
        GfxDriver::Instance.WaitBlitter();
        GfxDriver::Instance.SetFgColor(m_FgColor);
        IPoint screenPos(m_ScreenPos);
        IRect rectScr(rect);
        GfxDriver::Instance.SetWindow(gui.GetScreenFrame());
    
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
///
///////////////////////////////////////////////////////////////////////////////

void View::FillRect(const Rect& rect, uint16_t color)
{
    SetFgColor(color);
    FillRect(rect);
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Point View::ConvertToParent(const Point& point) const
{
    return point + GetLeftTop();
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void View::ConvertToParent(Point* point) const
{
    *point += GetLeftTop();
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Rect View::ConvertToParent(const Rect& rect) const
{
    return rect + GetLeftTop();
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void View::ConvertToParent(Rect* rect) const
{
    *rect += GetLeftTop();
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Point View::ConvertFromParent(const Point& point) const
{
    return point - GetLeftTop();
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void View::ConvertFromParent(Point* point) const
{
    *point -= GetLeftTop();
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Rect View::ConvertFromParent(const Rect& rect) const
{
    return rect - GetLeftTop();
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void View::ConvertFromParent(Rect* rect) const
{
    *rect -= GetLeftTop();
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Point View::ConvertToRoot(const Point& point) const
{
    return m_ScreenPos + point;
/*    Ptr<View> parent = m_Parent.Lock();
    if ( parent != nullptr ) {
	return( parent->ConvertToRoot( cPoint + GetLeftTop() ) );
    } else {
	return( cPoint );
    }*/
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void View::ConvertToRoot(Point* point) const
{
    *point += m_ScreenPos;
    
/*    Ptr<View> parent = m_Parent.Lock();
    if ( parent != nullptr )
    {
	*pcPoint += GetLeftTop();
	parent->ConvertToRoot( pcPoint );
    }*/
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Rect View::ConvertToRoot(const Rect& rect) const
{
    return rect + m_ScreenPos;
/*    Ptr<View> parent = m_Parent.Lock();
    if ( parent != nullptr ) {
	return( parent->ConvertToRoot( cRect + GetLeftTop() ) );
    } else {
	return( cRect );
    }*/
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void View::ConvertToRoot(Rect* rect) const
{
    *rect += m_ScreenPos;
/*    Ptr<View> parent = m_Parent.Lock();
    if ( parent != nullptr )
    {
	*pcRect += GetLeftTop();
	parent->ConvertToRoot( pcRect );
    }*/
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Point View::ConvertFromRoot(const Point& point) const
{
    return point - m_ScreenPos;
/*    Ptr<View> parent = m_Parent.Lock();
    if ( parent != nullptr ) {
	return( parent->ConvertFromRoot( cPoint - GetLeftTop() ) );
    } else {
	return( cPoint );
    }*/
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void View::ConvertFromRoot(Point* point) const
{
    *point -= m_ScreenPos;
/*    Ptr<View> parent = m_Parent.Lock();
    if ( parent != nullptr )
    {
	*pcPoint -= GetLeftTop();
	parent->ConvertFromRoot( pcPoint );
    }*/
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Rect View::ConvertFromRoot(const Rect& rect) const
{
    return rect - m_ScreenPos;
/*    Ptr<View> parent = m_Parent.Lock();
    if ( parent != nullptr ) {
	return( parent->ConvertFromRoot( cRect - GetLeftTop() ) );
    } else {
	return( cRect );
    }*/
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void View::ConvertFromRoot(Rect* rect) const
{
    *rect -= m_ScreenPos;
/*    Ptr<View> parent = m_Parent.Lock();
    if ( parent != nullptr )
    {
	*pcRect -= GetLeftTop();
	parent->ConvertFromRoot( pcRect );
    }*/
}
    
///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

bool View::HandleMouseDown(MouseButton_e button, const Point& position)
{
    for (Ptr<View> child = m_BottomChild; child != nullptr; child = child->m_HigherSibling)
    {
        if (child->m_Frame.DoIntersect(position))
        {
            Point childPos = position - child->m_Frame.LeftTop() - m_cScrollOffset;
            if (child->HandleMouseDown(button, childPos))
            {
                return true;
            }
        }
    }
    bool handled = OnMouseDown(button, position - m_cScrollOffset);
    if (handled)
    {
        gui.SetMouseDownView(ptr_tmp_cast(this));
    }
    return handled;
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void View::LinkChild( Ptr<View> pcChild, bool bTopmost )
{
    if ( pcChild->m_Parent.Lock() == nullptr )
    {
	pcChild->m_Parent = ptr_tmp_cast(this);
	if ( m_BottomChild == nullptr && m_TopChild == nullptr )
	{
	    m_BottomChild	= pcChild;
	    m_TopChild	= pcChild;
	    pcChild->m_LowerSibling  = nullptr;
	    pcChild->m_HigherSibling = nullptr;
	}
	else
	{
	    if ( bTopmost )
	    {
		if ( m_TopChild != nullptr ) {
		    m_TopChild->m_HigherSibling = pcChild;
		}

		pcChild->m_LowerSibling = m_TopChild;
		m_TopChild = pcChild;
		pcChild->m_HigherSibling = nullptr;
	    }
	    else
	    {
		if ( m_BottomChild != nullptr) {
		    m_BottomChild->m_LowerSibling = pcChild;
		}
		pcChild->m_HigherSibling = m_BottomChild;
		m_BottomChild		   = pcChild;
		pcChild->m_LowerSibling  = nullptr;
	    }
	}
        if (m_IsAttachedToScreen)
        {
            pcChild->HandleAttachedToScreen();
        }
    }
    else
    {
	printf( "ERROR : Attempt to add an view already belonging to a window\n" );
    }
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void View::UnlinkChild( Ptr<View> pcChild )
{
    if( pcChild->m_Parent.Lock() == this )
    {
	pcChild->m_Parent = nullptr;

	if ( pcChild == m_TopChild ) {
	    m_TopChild	= pcChild->m_LowerSibling;
	}
	if ( pcChild == m_BottomChild ) {
	    m_BottomChild	= pcChild->m_HigherSibling;
	}

	if ( pcChild->m_LowerSibling != nullptr) {
	    pcChild->m_LowerSibling->m_HigherSibling = pcChild->m_HigherSibling;
	}

	if ( pcChild->m_HigherSibling != nullptr ) {
	    pcChild->m_HigherSibling->m_LowerSibling = pcChild->m_LowerSibling;
	}

	pcChild->m_LowerSibling  = nullptr;
	pcChild->m_HigherSibling = nullptr;
    }
    else
    {
	printf( "ERROR : Attempt to remove a view not belonging to this window\n" );
    }
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void View::HandleAttachedToScreen()
{
    m_IsAttachedToScreen = true;
    PreAttachedToViewport();
    for (Ptr<View> child = m_BottomChild; child != nullptr; child = child->m_HigherSibling)
    {
        child->HandleAttachedToScreen();;
    }
//    PostAttachedToViewport();            
}

void View::HandlePostAttachedToScreen()
{
    for (Ptr<View> child = m_BottomChild; child != nullptr; child = child->m_HigherSibling)
    {
        child->HandlePostAttachedToScreen();;
    }
    PostAttachedToViewport();            
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

/*void View::OffsetScreenPos(const Point& delta)
{
    m_ScreenPos += delta;
    for (Ptr<View> child = m_BottomChild; child != nullptr; child = child->m_HigherSibling)
    {
        child->OffsetScreenPos(delta);
    }        
}*/

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void View::UpdateScreenPos()
{
    Ptr<View> parent = m_Parent.Lock();
    if (parent == nullptr)
    {
        m_ScreenPos = m_Frame.LeftTop();
    }
    else
    {
        m_ScreenPos = parent->m_ScreenPos + m_Frame.LeftTop();
    }
    for (Ptr<View> child = m_BottomChild; child != nullptr; child = child->m_HigherSibling)
    {
        child->UpdateScreenPos();
    }        
}

