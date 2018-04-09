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
#include <vector>

#include "System/Ptr/Ptr.h"
#include "System/Ptr/WeakPtr.h"
#include "system/Threads/EventHandler.h"
#include "System/Signals/SignalTarget.h"
#include "System/Signals/VFConnector.h"
#include "System/Math/Rect.h"
#include "System/Math/Point.h"
#include "System/GUI/Region.h"
#include "System/GUI/GUIEvent.h"
#include "Kernel/Drivers/RA8875Driver/GfxDriver.h"
#include "ApplicationServer/Protocol.h"
#include "Application.h"
#include "Font.h"
#include "Color.h"
#include "LayoutNode.h"

namespace os
{


/** \brief Flags controlling a View
 * \ingroup gui
 * \sa os::view_resize_flags, os::View
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

namespace ViewFlags
{
    enum Type
    {
        FULL_UPDATE_ON_H_RESIZE = 0x0001,  ///< Cause the entire view to be invalidated if made higher
        FULL_UPDATE_ON_V_RESIZE = 0x0002,  ///< Cause the entire view to be invalidated if made wider
        FULL_UPDATE_ON_RESIZE   = 0x0003,  ///< Cause the entire view to be invalidated if resized
        WILL_DRAW               = 0x0004,  ///< Tell the appserver that you want to render stuff to it
        TRANSPARENT             = 0x0008,  ///< Allow the parent view to render in areas covered by this view
        CLIENT_ONLY             = 0x0010,
        CLEAR_BACKGROUND        = 0x0020,  ///< Automatically clear new areas when windows are moved/resized
        DRAW_ON_CHILDREN        = 0x0040   ///< Setting this flag allows the view to render atop of all its childs
    };
}

/** \brief Flags controlling how to resize/move a view when the parent is resized.
 * \ingroup gui
 * \sa os::view_flags, os::View
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/
#if 0
enum view_resize_flags
{
    CF_FOLLOW_NONE   = 0x0000, ///< Neither the size nor the position is changed.
    CF_FOLLOW_LEFT   = 0x0001, ///< Left edge follows the parents left edge.
    CF_FOLLOW_RIGHT  = 0x0002, ///< Right edge follows the parents right edge.
    CF_FOLLOW_TOP    = 0x0004, ///< Top edge follows the parents top edge.
    CF_FOLLOW_BOTTOM = 0x0008, ///< Bottom edge follows the parents bottom edge.
    CF_FOLLOW_ALL    = 0x000F, ///< All edges follows the corresponding edge in the parent
      /**
       * If the CF_FOLLOW_LEFT is set the right edge follows the parents center.
       * if the CF_FOLLOW_RIGHT is set the left edge follows the parents center.
       */
    CF_FOLLOW_H_MIDDLE = 0x0010,
      /**
       * If the CF_FOLLOW_TOP is set the bottom edge follows the parents center.
       * if the CF_FOLLOW_BOTTOM is set the top edge follows the parents center.
       */
    CF_FOLLOW_V_MIDDLE = 0x0020,
    CF_FOLLOW_SPECIAL  = 0x0040,
    CF_FOLLOW_MASK     = 0x007f
};
#endif

enum drawing_mode
{
    DM_COPY,
    DM_OVER,
    DM_INVERT,
    DM_ERASE,
    DM_BLEND,
    DM_ADD,
    DM_SUBTRACT,
    DM_MIN,
    DM_MAX,
    DM_SELECT
};

template<typename ViewType>
class ViewBase : public EventHandler, public SignalTarget
{
public:
    typedef std::vector<Ptr<ViewType>> ChildList_t;

    ViewBase(const String& name, const Rect& frame, const Point& scrollOffset, uint32_t flags, int32_t hideCount, Color eraseColor, Color bgColor, Color fgColor)
        : EventHandler(name)
        , m_Frame(frame)
        , m_IFrame(frame)
        , m_ScrollOffset(scrollOffset)
        , m_Flags(flags)
        , m_HideCount(hideCount)
        , m_EraseColor(eraseColor)
        , m_BgColor(bgColor)
        , m_FgColor(fgColor) {}

    Ptr<ViewType>       GetParent()       { return m_Parent.Lock(); }
    Ptr<const ViewType> GetParent() const { return m_Parent.Lock(); }

    const ChildList_t& GetChildList() const { return m_ChildrenList; }
        
    typename ChildList_t::iterator begin() { return m_ChildrenList.begin(); }
    typename ChildList_t::iterator end()   { return m_ChildrenList.end(); }
        
    typename ChildList_t::const_iterator begin() const { return m_ChildrenList.begin(); }
    typename ChildList_t::const_iterator end() const   { return m_ChildrenList.end(); }
        
    typename ChildList_t::iterator       GetChildIterator(Ptr<ViewType> child)       { return std::find(m_ChildrenList.begin(), m_ChildrenList.end(), child); }
    typename ChildList_t::const_iterator GetChildIterator(Ptr<ViewType> child) const { return std::find(m_ChildrenList.begin(), m_ChildrenList.end(), child); }

    typename ChildList_t::reverse_iterator       GetChildRIterator(Ptr<ViewType> child)       { return std::find(m_ChildrenList.rbegin(), m_ChildrenList.rend(), child); }
    typename ChildList_t::const_reverse_iterator GetChildRIterator(Ptr<ViewType> child) const { return std::find(m_ChildrenList.rbegin(), m_ChildrenList.rend(), child); }
    
    int32_t GetChildIndex(Ptr<ViewType> child) const
    {
        auto i = GetChildIterator(child);
        if (i != m_ChildrenList.end()) {
            return std::distance(m_ChildrenList.begin(), i);
        } else {
            return -1;
        }
    }

//    void       SetFlags(uint32_t flags);
    uint32_t   GetFlags() const { return m_Flags; }
    bool       HasFlag(ViewFlags::Type flag) const { return (m_Flags & uint32_t(flag)) != 0; }
    bool       HasFlagsAll(uint32_t mask) const { return (m_Flags & mask) == mask; }
    

    const Rect&         GetFrame() const { return m_Frame; }
    Rect                GetBounds() const { return m_Frame.Bounds() /*- Point(m_Frame.left, m_Frame.top)*/ - m_ScrollOffset; }
    Rect                GetNormalizedBounds() const { return m_Frame.Bounds(); }

    const IRect&        GetIFrame() const { return m_IFrame; }
    IRect               GetIBounds() const { return m_IFrame.Bounds() /*- Point(m_Frame.left, m_Frame.top)*/ - IPoint(m_ScrollOffset); }
    IRect               GetNormalizedIBounds() const { return m_IFrame.Bounds(); }
        
    Point               GetLeftTop() const  { return Point( m_Frame.left, m_Frame.top ); }
    IPoint              GetILeftTop() const { return IPoint( m_IFrame.left, m_IFrame.top ); }
        
    Color               GetFgColor() const { return m_FgColor; }
    Color               GetBgColor() const { return m_BgColor; }
    Color               GetEraseColor() const { return m_EraseColor; }

    
      // Coordinate conversions:
    Point               ConvertToParent(const Point& point) const   { return point + GetLeftTop(); }
    void                ConvertToParent(Point* point) const         { *point += GetLeftTop(); }
    Rect                ConvertToParent(const Rect& rect) const     { return rect + GetLeftTop(); }
    void                ConvertToParent(Rect* rect) const           { *rect += GetLeftTop(); }
    Point               ConvertFromParent(const Point& point) const { return point - GetLeftTop(); }
    void                ConvertFromParent(Point* point) const       { *point -= GetLeftTop(); }
    Rect                ConvertFromParent(const Rect& rect) const   { return rect - GetLeftTop(); }
    void                ConvertFromParent(Rect* rect) const         { *rect -= GetLeftTop(); }
    Point               ConvertToRoot(const Point& point) const     { return m_ScreenPos + point; }
    void                ConvertToRoot(Point* point) const           { *point += m_ScreenPos; }
    Rect                ConvertToRoot(const Rect& rect) const       { return rect + m_ScreenPos; }
    void                ConvertToRoot(Rect* rect) const             { *rect += m_ScreenPos; }
    Point               ConvertFromRoot(const Point& point) const   { return point - m_ScreenPos; }
    void                ConvertFromRoot(Point* point) const         { *point -= m_ScreenPos; }
    Rect                ConvertFromRoot(const Rect& rect) const     { return rect - m_ScreenPos; }
    void                ConvertFromRoot(Rect* rect) const           { *rect -= m_ScreenPos; }
    //Point               GetScrollOffset() const { return( m_ScrollOffset ); }
    
    static Ptr<ViewType> GetOpacParent(Ptr<ViewType> view, IRect* frame)
    {
        while(view != nullptr && (view->m_Flags & ViewFlags::TRANSPARENT))
        {
            if (frame != nullptr) {
                *frame += view->m_IFrame.LeftTop();
            }
            view = view->GetParent();
        }
        return view;
    }        
protected:
    friend class GUI;
    friend class Application;
    friend class ApplicationServer;
    friend class ServerApplication;

    void LinkChild(Ptr<ViewType> child, bool topmost);
    void UnlinkChild(Ptr<ViewType> child);
    
    void Added(ViewBase* parent, int hideCount, int level)
    {
        m_HideCount += hideCount;
        m_Level = level;
        if (parent == nullptr) {
            m_ScreenPos = m_Frame.LeftTop();
        } else {
            m_ScreenPos = parent->m_ScreenPos + m_Frame.LeftTop();
        }
        for (Ptr<ViewBase> child : m_ChildrenList) {
            child->Added(this, hideCount, level + 1);
        }
    }

    Rect m_Frame = Rect(0.0f, 0.0f, 0.0f, 0.0f);
    IRect m_IFrame; // Frame rectangle relative to our parent
    Point  m_ScrollOffset;
    uint32_t m_Flags = 0;    

    Point m_ScreenPos = Point(0.0f, 0.0f);
    WeakPtr<ViewType> m_Parent;
    
    ChildList_t       m_ChildrenList;
    
    Point             m_PenPosition;

    int               m_HideCount = 0;
    int               m_Level = 0;

    Color             m_EraseColor = Color(0xffffffff);
    Color             m_BgColor    = Color(0xffffffff);
    Color             m_FgColor    = Color(0xff000000);
    
    ViewBase(const ViewBase&) = delete;
    ViewBase& operator=(const ViewBase&) = delete;
};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template<typename ViewType>
void ViewBase<ViewType>::LinkChild(Ptr<ViewType> child, bool topmost)
{
    if ( child->m_Parent.Lock() == nullptr )
    {
        child->m_Parent = ptr_tmp_cast(static_cast<ViewType*>(this));
        if (topmost) {
            m_ChildrenList.push_back(child);
        } else {
            m_ChildrenList.insert(m_ChildrenList.begin(), child);
        }            
        child->Added(this, m_HideCount, m_Level + 1);
        child->HandleAddedToParent(ptr_tmp_cast(static_cast<ViewType*>(this)));
    }
    else
    {
        printf( "ERROR : Attempt to add a view already belonging to a window\n" );
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template<typename ViewType>
void ViewBase<ViewType>::UnlinkChild(Ptr<ViewType> child)
{
    if(child->m_Parent.Lock() == static_cast<ViewType*>(this))
    {
        child->m_Parent = nullptr;

        auto i = GetChildIterator(child);
        if (i != m_ChildrenList.end()) {
            m_ChildrenList.erase(i);
        } else {
            printf("ERROR: ViewBase::UnlinkChildren() failed to find view in children list.\n");
        }
        child->HandleRemovedFromParent(ptr_tmp_cast(static_cast<ViewType*>(this)));
    }
    else
    {
        printf( "ERROR : Attempt to remove a view not belonging to this window\n" );
    }
}

class View : public ViewBase<View>
{
public:
    View(const String& name, Ptr<View> parent = nullptr, uint32_t flags = 0);
    virtual ~View();
    
    Application* GetApplication();
    
    virtual void AttachedToScreen() {}
    virtual void AllAttachedToScreen() {}
    virtual void DetachedFromScreen() {}
    virtual void AllDetachedFromScreen() {}

    Ptr<LayoutNode> GetLayoutNode() const;
    void            SetLayoutNode(Ptr<LayoutNode> node);
    void            InvalidateLayout();

    void            AdjustPrefSize(Point* minSize, Point* maxSize);

//    virtual void Activated(bool isActive);
    //    virtual void WindowActivated( bool bIsActive );

    virtual void Paint(const Rect& updateRect) {}

    virtual bool OnMouseDown(MouseButton_e button, const Point& position);
    virtual bool OnMouseUp(MouseButton_e button, const Point& position);
    virtual bool OnMouseMove(MouseButton_e button, const Point& position);

    virtual void  FrameMoved(const Point& delta);
    virtual void  FrameSized(const Point& delta);
    virtual void  ViewScrolled(const Point& delta);
    virtual void  FontChanged(Ptr<Font> newFont);
    
    virtual Point GetPreferredSize(bool largest) const;
    virtual float GetWheight() const;
    virtual Point GetContentSize() const;

//    virtual void WheelMoved( const Point& cDelta );

    void AddChild(Ptr<View> child);
    void RemoveChild(Ptr<View> child);
    bool RemoveThis();
    
    Ptr<View>  GetChildAt(const Point& pos);
    Ptr<View>  GetChildAt(int index);

    void       SetResizeMask( uint32_t nFlags );
    uint32_t   GetResizeMask() const;
    
    void       Show( bool bVisible = true );
    void       Hide() { Show( false ); }
    bool       IsVisible() const;
//    virtual void MakeFocus( bool bFocus = true );
//    virtual bool HasFocus() const;

//    Rect GetFrame() const;
    float  Width() const;
    float  Height() const;
//    Point  GetLeftTop() const;

    virtual void        SetFrame(const Rect& frame, bool bNotifyServer = true);
    void                Move(const Point& delta) { SetFrame(m_Frame + delta); }
/*    virtual void MoveBy( const Point& cDelta );
    virtual void MoveBy( float vDeltaX, float vDeltaY );
    virtual void MoveTo( const Point& cPos );
    virtual void MoveTo( float x, float y );

    virtual void ResizeBy( const Point& cDelta );
    virtual void ResizeBy( float vDeltaW, float vDeltaH );
    virtual void ResizeTo( const Point& cSize );
    virtual void ResizeTo( float W, float H );*/

    void                SetDrawingRegion( const Region& cReg );
    void                ClearDrawingRegion();
    void                SetShapeRegion( const Region& cReg );
    void                ClearShapeRegion();
    
//    virtual int ToggleDepth();

    void                Invalidate( const Rect& cRect, bool bRecurse = false );
    void                Invalidate( bool bRecurse = false );

    void                SetDrawingMode( drawing_mode nMode );
    drawing_mode        GetDrawingMode() const;
    void                SetFont(Ptr<Font> font);
    Ptr<Font>           GetFont() const;

    bool            HandleMouseDown(MouseButton_e button, const Point& position);
    void            HandleMouseUp(MouseButton_e button, const Point& position);
    void            HandleMouseMove(MouseButton_e button, const Point& position);
    
    void            SetFgColor(int red, int green, int blue, int alpha = 255) { SetFgColor(Color(red, green, blue, alpha)); }
    void            SetFgColor(Color color) { m_FgColor = color; Post<ASViewSetFgColor>(color); }
    void            SetBgColor(int red, int green, int blue, int alpha = 255) { SetBgColor(Color(red, green, blue, alpha)); }
    void            SetBgColor(Color color) { m_BgColor = color; Post<ASViewSetBgColor>(color); }
    void            SetEraseColor(int red, int green, int blue, int alpha = 255) { SetEraseColor(Color(red, green, blue, alpha)); }
    void            SetEraseColor(Color color) { m_EraseColor = color; Post<ASViewSetEraseColor>(color); }

    void            MovePenTo(const Point& pos)                         { m_PenPosition = pos; Post<ASViewMovePenTo>(pos); }
    void            MovePenTo(float x, float y)                         { MovePenTo(Point(x, y)); }
    void            MovePenBy(const Point& pos)                         { MovePenTo(m_PenPosition + pos); }
    void            MovePenBy(float x, float y)                         { MovePenBy(Point(x, y)); }
    Point           GetPenPosition() const                              { return m_PenPosition; }
    void            DrawLine(const Point& toPos)                        { Post<ASViewDrawLine1>(toPos); }
    void            DrawLine(const Point& fromPos, const Point& toPos ) { Post<ASViewDrawLine2>(fromPos, toPos); }
    void            DrawLine(float x1, float y1, float x2, float y2)    { DrawLine(Point(x1, y1), Point(x2, y2)); }
    void            FillRect( const Rect& rect )                        { Post<ASViewFillRect>(rect); }
    void            FillRect( const Rect& rect, Color color ); // WARNING: Will leave FgColor at color

    void            FillCircle(const Point& position, float radius) { Post<ASViewFillCircle>(position, radius); }
    void            DrawString(const String& string, float maxWidth = 100000.0f, uint8_t flags = 0) { Post<ASViewDrawString>(string, maxWidth, flags); }

    virtual void    ScrollBy(const Point& delta)           { Post<ASViewScrollBy>(delta); }
    virtual void    ScrollBy(float vDeltaX, float vDeltaY) { ScrollBy(Point(vDeltaX, vDeltaY)); }
    virtual void    ScrollTo(Point topLeft)                { ScrollBy(topLeft - m_ScrollOffset); }
    virtual void    ScrollTo(float x, float y)             { ScrollTo(Point(x, y)); }
        
    Point           GetScrollOffset() const                { return m_ScrollOffset; }
    void            CopyRect(const Rect& srcRect, const Point& dstPos) { Post<ASViewCopyRect>(srcRect, dstPos); }
    
    //    void DrawBitmap( const Bitmap* pcBitmap, const Rect& cSrcRect, const Rect& cDstRect );
    void            EraseRect( const Rect& cRect );
    void            DrawFrame( const Rect& cRect, uint32_t nFlags );

        
    void            DrawBevelBox(Rect frame, bool raised);

    FontHeight      GetFontHeight() const;
    float           GetStringWidth(const char* string, size_t length) const;
    float           GetStringWidth(const String& string) const { return GetStringWidth(string.data(), string.length()); }

    void            Flush();
    void            Sync();

    VFConnector<bool, MouseButton_e, const Point&> VFMouseDown;
    VFConnector<bool, MouseButton_e, const Point&> VFMouseUp;
    VFConnector<bool, const Point&>                VFMouseMoved;
    
private:
    friend class Application;
    friend class ViewBase<View>;

    template<typename SIGNAL, typename... ARGS>
    void Post(ARGS&&... args) {
        Application* app = GetApplication();
        if (app != nullptr) {
            SIGNAL::Sender::Emit(app, &Application::AllocMessageBuffer, m_ServerHandle, args...);
        }
    }        

    void HandleAddedToParent(Ptr<View> parent);
    void HandleRemovedFromParent(Ptr<View> parent);


    void HandleDetachedFromScreen();

    void UpdatePosition(bool forceServerUpdate)
    {
        Point newOffset;
        {
            Ptr<View> parent = m_Parent.Lock();
            if (parent != nullptr && parent->HasFlag(ViewFlags::CLIENT_ONLY)) {
                newOffset = parent->m_PositionOffset + parent->m_Frame.LeftTop();
            } else {
                newOffset = Point(0.0f, 0.0f);
            }
        }            
        if (forceServerUpdate || newOffset != m_PositionOffset)
        {
            m_PositionOffset = newOffset;
            if (m_ServerHandle != -1 && !HasFlag(ViewFlags::CLIENT_ONLY))
            {
                GetApplication()->SetViewFrame(m_ServerHandle, m_Frame + m_PositionOffset);
            }                    
        }
        for (Ptr<View> child : m_ChildrenList) {
            child->UpdatePosition(false);
        }
    }

    void HandlePaint(const Rect& updateRect);
    
    void       SetServerHandle(handler_id handle) { m_ServerHandle = handle; }
    handler_id GetServerHandle() const            { return m_ServerHandle; }
        
    void ConstrictRectangle(Rect* rect, const Point& offset);

    handler_id m_ServerHandle = -1;
    
    Ptr<LayoutNode> m_LayoutNode;
    
    Point m_PositionOffset; // Offset relative to first parent that is not client only.
    
    bool m_DidScrollRect = false;
    int  m_BeginPainCount = 0;

    Ptr<Font> m_Font = ptr_new<Font>(kernel::GfxDriver::e_FontLarge);
    
    static WeakPtr<View> s_MouseDownView;
    
    ASPaintView::Receiver RSPaintView;
    
    ASHandleMouseDown::Receiver RSHandleMouseDown;
    ASHandleMouseUp::Receiver   RSHandleMouseUp;
    ASHandleMouseMove::Receiver RSHandleMouseMove;
};

} // namespace
