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

#pragma once


#include <stdint.h>

#include "System/Ptr/Ptr.h"
#include "System/Ptr/WeakPtr.h"
#include "System/Signals/SignalTarget.h"
#include "System/Signals/VFConnector.h"
#include "System/Math/Rect.h"
#include "System/Math/Point.h"
#include "System/GUI/Region.h"
#include "System/GUI/GUIEvent.h"
#include "Kernel/Drivers/RA8875Driver/GfxDriver.h"

//#include "RA8875Driver/GfxDriver.h"

/** \brief Flags controlling a View
 * \ingroup gui
 * \sa os::view_resize_flags, os::View
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

enum view_flags
{
    WID_FULL_UPDATE_ON_H_RESIZE	= 0x0001,  ///< Cause the entire view to be invalidated if made higher
    WID_FULL_UPDATE_ON_V_RESIZE	= 0x0002,  ///< Cause the entire view to be invalidated if made wider
    WID_FULL_UPDATE_ON_RESIZE	= 0x0003,  ///< Cause the entire view to be invalidated if resized
    WID_WILL_DRAW		= 0x0004,  ///< Tell the appserver that you want to render stuff to it
    WID_TRANSPARENT		= 0x0008,  ///< Allow the parent view to render in areas covered by this view
    WID_CLEAR_BACKGROUND	= 0x0010,  ///< Automatically clear new areas when windows are moved/resized
    WID_DRAW_ON_CHILDREN	= 0x0020   ///< Setting this flag allows the view to render atop of all its childs
};

/** \brief Flags controlling how to resize/move a view when the parent is resized.
 * \ingroup gui
 * \sa os::view_flags, os::View
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

enum view_resize_flags
{
    CF_FOLLOW_NONE	= 0x0000, ///< Neighter the size nor the position is changed.
    CF_FOLLOW_LEFT	= 0x0001, ///< Left edge follows the parents left edge.
    CF_FOLLOW_RIGHT	= 0x0002, ///< Right edge follows the parents right edge.
    CF_FOLLOW_TOP	= 0x0004, ///< Top edge follows the parents top edge.
    CF_FOLLOW_BOTTOM	= 0x0008, ///< Bottom edge follows the parents bottom edge.
    CF_FOLLOW_ALL	= 0x000F, ///< All edges follows the corresponding edge in the parent
      /**
       * If the CF_FOLLOW_LEFT is set the right edge follows the parents center.
       * if the CF_FOLLOW_RIGHT is set the left edge follows the parents center.
       */
    CF_FOLLOW_H_MIDDLE	= 0x0010,
      /**
       * If the CF_FOLLOW_TOP is set the bottom edge follows the parents center.
       * if the CF_FOLLOW_BOTTOM is set the top edge follows the parents center.
       */
    CF_FOLLOW_V_MIDDLE	= 0x0020,
    CF_FOLLOW_SPECIAL	= 0x0040,
    CF_FOLLOW_MASK	= 0x007f
};


class View : public PtrTarget, public SignalTarget
{
public:
    View();
    ~View();

/*    virtual void	AttachedToWindow();
    virtual void	AllAttached();
    virtual void	DetachedFromWindow();
    virtual void	AllDetached();
    virtual void	Activated( bool bIsActive );
    virtual void	WindowActivated( bool bIsActive );
*/
    virtual void	PostAttachedToViewport() {}
    virtual void	PreAttachedToViewport() {}

    void AddChild(Ptr<View> child);
    void RemoveChild(Ptr<View> child);    
    void SetFrame(const Rect& frame);
    const Rect& GetFrame() const;
    const Rect GetBounds() const { return m_Frame.Bounds(); }
        
    Point  GetLeftTop() const  { return Point( m_Frame.left, m_Frame.top ); }
    IPoint GetILeftTop() const { return IPoint( m_cIFrame.left, m_cIFrame.top ); }
        
    void Move(const Point& delta);
    void Show(bool doShow);

    //void SetCursor(float x, float y);
    //void SetCursor(const Point& pos) { SetCursor(pos.x, pos.y); }
        
    void DrawBevelBox(Rect frame, bool raised);
//    void FillRect(const Rect& frame, uint16_t color);
//    void DrawLine(uint16_t color, float x1, float y1, float x2, float y2);

    void DrawString(const char* string, uint32_t strLength, float maxWidth, uint8_t flags);

//    void Invalidate();
    virtual void Render() {}


    virtual bool OnMouseDown(MouseButton_e button, const Point& position);
    virtual bool OnMouseUp(MouseButton_e button, const Point& position);
    virtual bool OnMouseMove(MouseButton_e button, const Point& position);

    void		SetFgColor( int red, int green, int blue, int alpha = 255 ) { SetFgColor(LCD_RGB(red, green, blue)); }
    void		SetFgColor( uint16_t color ) { m_FgColor = color; }
    uint16_t		GetFgColor() const { return m_FgColor; }

    void		SetBgColor( int red, int green, int blue, int alpha = 255 ) { SetBgColor(LCD_RGB(red, green, blue)); }
    void		SetBgColor( uint16_t color ) { m_BgColor = color; }
    uint16_t		GetBgColor() const { return m_BgColor; }

//    void		SetEraseColor( int nRed, int nGreen, int nBlue, int nAlpha = 255 );
//    void		SetEraseColor( Color32_s sColor );
//    Color32_s		GetEraseColor() const;

    void SetDrawRegion(Region* pcReg);
    void SetShapeRegion(Region* pcReg);
    void Invalidate( const IRect& cRect);
    void Invalidate(bool bReqursive = false);
    void InvalidateNewAreas();
    void MoveChilds();
    void SwapRegions(bool bForce);
    void RebuildRegion(bool bForce);
    void ClearDirtyRegFlags();
    void UpdateRegions(bool bForce = true, bool bRoot = true);
    Region* GetRegion();
    void PutRegion(Region* pcReg);
    void BeginUpdate();
    void EndUpdate();

    void		MovePenTo(const Point& pos) { m_PenPosition = pos; }
    void		MovePenTo(float x, float y) { MovePenTo( Point( x, y ) ); }
    void		MovePenBy(const Point& pos) { m_PenPosition += pos; }
    void		MovePenBy(float x, float y) { MovePenBy( Point( x, y ) ); }
    Point		GetPenPosition() const { return m_PenPosition; }
    void		DrawLine( const Point& cToPoint );
    void		DrawLine( const Point& cFromPnt, const Point& cToPnt );
    void		DrawLine(float x1, float y1, float x2, float y2) { DrawLine(Point(x1, y1), Point(x2, y2)); }
        
    void                FillCircle(const Point& position, float radius);
    
    virtual void	ScrollBy( const Point& cDelta );
    virtual void	ScrollBy( float vDeltaX, float vDeltaY ) { ScrollBy( Point( vDeltaX, vDeltaY ) );	}
    virtual void	ScrollTo( Point topLeft ) { ScrollBy(topLeft - m_cScrollOffset); }
    virtual void	ScrollTo( float x, float y ) { ScrollTo( Point( x, y ) );	}
    Point		GetScrollOffset() const { return m_cScrollOffset; }
    IPoint		GetIScrollOffset() const { return( m_cIScrollOffset ); }
    void		ScrollRect(const Rect& srcRect, const Point& dstPos);
    
    void		FillRect( const Rect& rect );
    void		FillRect( const Rect& rect, uint16_t color );	// WARNING: Will leave HiColor at sColor
//    void		DrawBitmap( const Bitmap* pcBitmap, const Rect& cSrcRect, const Rect& cDstRect );
    void		EraseRect( const Rect& cRect );
    void		DrawFrame( const Rect& cRect, uint32_t nFlags );

      // Coordinate conversions:
    Point		ConvertToParent( const Point& cPoint ) const;
    void		ConvertToParent( Point* pcPoint ) const;
    Rect		ConvertToParent( const Rect& cRect ) const;
    void		ConvertToParent( Rect* pcRect ) const;
    Point		ConvertFromParent( const Point& cPoint ) const;
    void		ConvertFromParent( Point* pcPoint ) const;
    Rect		ConvertFromParent( const Rect& cRect ) const;
    void		ConvertFromParent( Rect* pcRect ) const;
    Point		ConvertToRoot( const Point& cPoint )	const;
    void		ConvertToRoot( Point* pcPoint ) const;
    Rect		ConvertToRoot( const Rect& cRect ) const;
    void 		ConvertToRoot( Rect* pcRect ) const;
    Point		ConvertFromRoot( const Point& cPoint ) const;
    void		ConvertFromRoot( Point* pcPoint ) const;
    Rect		ConvertFromRoot( const Rect& cRect ) const;
    void		ConvertFromRoot( Rect* pcRect ) const;
    //Point		GetScrollOffset() const { return( m_cScrollOffset ); }


    VFConnector<bool, MouseButton_e, const Point&> VFMouseDown;
    VFConnector<bool, MouseButton_e, const Point&> VFMouseUp;
    VFConnector<bool, const Point&>                VFMouseMoved;


    bool HandleMouseDown(MouseButton_e button, const Point& position);


private:
    friend class GUI;
    
    void LinkChild( Ptr<View> pcChild, bool bTopmost );
    void UnlinkChild( Ptr<View> pcChild );

    void Added(int hideCount, int level);
    void Paint( const IRect& cUpdateRect, bool bUpdate = false );

    void DeleteRegions();
public:
    void UpdateIfNeeded(bool force);
private:
    void MarkModified(const IRect& rect);
    void SetDirtyRegFlags();

    void HandleAttachedToScreen();
    void HandlePostAttachedToScreen();
//    void OffsetScreenPos(const Point& delta);
    void UpdateScreenPos();

    uint32_t m_Flags = 0;    
    Rect m_Frame = Rect(0.0f, 0.0f, 0.0f, 0.0f);
    IRect m_cIFrame;		// Frame rectangle relative to our parent
    Point  m_cScrollOffset;
    IPoint m_cIScrollOffset;

    Point m_ScreenPos = Point(0.0f, 0.0f);
    WeakPtr<View> m_Parent; // = nullptr;
    Ptr<View> m_TopChild; // = nullptr;
    Ptr<View> m_BottomChild; // = nullptr;
    Ptr<View> m_LowerSibling; // = nullptr;
    Ptr<View> m_HigherSibling; // = nullptr;
    
    
    uint16_t m_FgColor;
    uint16_t m_BgColor;
    Point    m_PenPosition;

    int	     m_HideCount = 0;
    int      m_Level = 0;
    
    bool     m_IsAttachedToScreen = false;
    
    IPoint  m_cDeltaMove;		// Relative movement since last region update
    IPoint  m_cDeltaSize;		// Relative sizing since last region update

    Region* m_pcDrawConstrainReg = nullptr;	// User specified "local" clipping list.
    Region* m_pcShapeConstrainReg = nullptr;	// User specified clipping list specifying the shape of the layer.

    Region* m_pcVisibleReg = nullptr;		// Visible areas, not including non-transparent children
    Region* m_pcFullReg = nullptr;		// All visible areas, including children
    Region* m_PrevVisibleReg = nullptr;         // Temporary storage for m_pcVisibleReg during region rebuild
    Region* m_PrevFullReg = nullptr;            // Temporary storage for m_pcFullReg during region rebuild
    Region* m_pcDrawReg = nullptr;		// Only valid between BeginPaint()/EndPaint()
    Region* m_pcDamageReg = nullptr;		// Contains areas made visible since the last M_PAINT message sent
    Region* m_pcActiveDamageReg = nullptr;
    
    bool	m_bHasInvalidRegs = true;	// True if something made our clipping region invalid
    bool	m_bIsUpdating = false;		// True while we paint areas from the damage list
    
    View( const View &c );
    View& operator=( const View &c );
};

