// This file is part of PadOS.
//
// Copyright (C) 1999-2020 Kurt Skauen <http://kavionic.com/>
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

#include "Ptr/Ptr.h"
#include "Ptr/WeakPtr.h"
#include "Threads/EventHandler.h"
#include "Signals/SignalTarget.h"
#include "Signals/VFConnector.h"
#include "Math/Rect.h"
#include "Math/Point.h"
#include "App/Application.h"
#include "GUI/ViewBase.h"
#include "GUI/Region.h"
#include "GUI/GUIEvent.h"
#include "GUI/Font.h"
#include "GUI/Color.h"
#include "GUI/LayoutNode.h"
#include "ApplicationServer/Protocol.h"
#include "Kernel/Drivers/RA8875Driver/GfxDriver.h"
#include "ViewFactoryContext.h"

namespace pugi
{
class xml_node;
}

#define DEFINE_FLAG_MAP_ENTRY(NAMESPACE, FLAG) {#FLAG, NAMESPACE::FLAG}

namespace os
{

class ButtonGroup;


Color get_standard_color(StandardColorID colorID);
void  set_standard_color(StandardColorID colorID, Color color);


class View : public ViewBase<View>
{
public:
    View(const String& name, Ptr<View> parent = nullptr, uint32_t flags = 0);
    View(ViewFactoryContext* context, Ptr<View> parent, const pugi::xml_node& xmlData);
    View(Ptr<View> parent, handler_id serverHandle, const String& name, const Rect& frame);
    virtual ~View();
    
    Application* GetApplication();
    handler_id   GetServerHandle() const { return m_ServerHandle; }
    
    virtual void AttachedToScreen() {}
    virtual void AllAttachedToScreen() {}
    virtual void DetachedFromScreen() {}
    virtual void AllDetachedFromScreen() {}

    Ptr<LayoutNode> GetLayoutNode() const;
    void            SetLayoutNode(Ptr<LayoutNode> node);
    
    void            SetBorders(const Rect& border);
    void            SetBorders(float l, float t, float r, float b) { SetBorders(Rect(l, t, r, b)); }
    Rect            GetBorders() const;

    float           GetWheight() const;
    void            SetWheight(float wheight);
    
    void            SetHAlignment(Alignment alignment);
    void            SetVAlignment(Alignment alignment);
    Alignment       GetHAlignment() const;
    Alignment       GetVAlignment() const;
    
    void            SetWidthOverride(PrefSizeType sizeType, SizeOverride when, float size);
    void            SetHeightOverride(PrefSizeType sizeType, SizeOverride when, float size);
    
    void            AddToWidthRing(Ptr<View> ring);
    void            RemoveFromWidthRing();

    void            AddToHeightRing(Ptr<View> ring);
    void            RemoveFromHeightRing();
        
    void            InvalidateLayout();
    void            UpdateLayout();

//    virtual void Activated(bool isActive);
    //    virtual void WindowActivated( bool bIsActive );

    virtual void Paint(const Rect& updateRect) { EraseRect(updateRect); }

    virtual bool OnMouseDown(MouseButton_e button, const Point& position);
    virtual bool OnMouseUp(MouseButton_e button, const Point& position);
    virtual bool OnMouseMove(MouseButton_e button, const Point& position);

    virtual void  FrameMoved(const Point& delta);
    virtual void  FrameSized(const Point& delta);
    virtual void  ViewScrolled(const Point& delta);
    virtual void  FontChanged(Ptr<Font> newFont);
    
    virtual void CalculatePreferredSize(Point* minSize, Point* maxSize, bool includeWidth, bool includeHeight) const;
    
    Point GetPreferredSize(PrefSizeType sizeType) const;
    virtual Point GetContentSize() const;

    void PreferredSizeChanged();
    void ContentSizeChanged();


//    virtual void WheelMoved( const Point& cDelta );

    void AddChild(Ptr<View> child);
    void RemoveChild(Ptr<View> child);
    bool RemoveThis();
    
    Ptr<View>  GetChildAt(const Point& pos);
    Ptr<View>  GetChildAt(size_t index);

    void       Show(bool visible = true);
    void       Hide() { Show(false); }
    bool       IsVisible() const;
	virtual void MakeFocus(bool bFocus = true) {}
	virtual bool HasFocus() const { return true; }

    float  Width() const;
    float  Height() const;

    virtual void        SetFrame(const Rect& frame);
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
    
    virtual void ToggleDepth() { Post<ASViewToggleDepth>(); }

    void                Invalidate( const Rect& cRect, bool bRecurse = false );
    void                Invalidate( bool bRecurse = false );

    void                SetDrawingMode( drawing_mode nMode );
    drawing_mode        GetDrawingMode() const;
    void                SetFont(Ptr<Font> font);
    Ptr<Font>           GetFont() const;

    bool            HandleMouseDown(MouseButton_e button, const Point& position);
    void            HandleMouseUp(MouseButton_e button, const Point& position);
    void            HandleMouseMove(MouseButton_e button, const Point& position);
    
    void            SetFgColor(int red, int green, int blue, int alpha = 255)	    { SetFgColor(Color(red, green, blue, alpha)); }
    void            SetFgColor(Color color)					    { m_FgColor = color; Post<ASViewSetFgColor>(color); }
    void            SetFgColor(StandardColorID colorID)				    { SetFgColor(get_standard_color(colorID)); }

    void            SetBgColor(int red, int green, int blue, int alpha = 255)	    { SetBgColor(Color(red, green, blue, alpha)); }
    void            SetBgColor(Color color)					    { m_BgColor = color; Post<ASViewSetBgColor>(color); }
    void            SetBgColor(StandardColorID colorID)				    { SetBgColor(get_standard_color(colorID)); }

    void            SetEraseColor(int red, int green, int blue, int alpha = 255)    { SetEraseColor(Color(red, green, blue, alpha)); }
    void            SetEraseColor(Color color)					    { m_EraseColor = color; Post<ASViewSetEraseColor>(color); }
    void            SetEraseColor(StandardColorID colorID)			    { SetEraseColor(get_standard_color(colorID)); }

    void            MovePenTo(const Point& pos)                        { m_PenPosition = pos; Post<ASViewMovePenTo>(pos); }
    void            MovePenTo(float x, float y)                        { MovePenTo(Point(x, y)); }
    void            MovePenBy(const Point& pos)                        { MovePenTo(m_PenPosition + pos); }
    void            MovePenBy(float x, float y)                        { MovePenBy(Point(x, y)); }
    Point           GetPenPosition() const                             { return m_PenPosition; }
    void            DrawLine(const Point& toPos)                       { Post<ASViewDrawLine1>(toPos); }
    void            DrawLine(const Point& fromPos, const Point& toPos) { Post<ASViewDrawLine2>(fromPos, toPos); }
    void            DrawLine(float x, float y)			               { DrawLine(Point(x, y)); }
    void            DrawLine(float x1, float y1, float x2, float y2)   { DrawLine(Point(x1, y1), Point(x2, y2)); }
    void            DrawRect(const Rect& frame)
    {
        MovePenTo(Point(frame.left, frame.top));
        DrawLine(Point(frame.right - 1.0f, frame.top));
        DrawLine(Point(frame.right - 1.0f, frame.bottom - 1.0f));
        DrawLine(Point(frame.left, frame.bottom - 1.0f));
        DrawLine(Point(frame.left, frame.top));
    }        
    void            FillRect(const Rect& rect)                         { Post<ASViewFillRect>(rect, m_FgColor); }
    void            FillRect(const Rect& rect, Color color)            { Post<ASViewFillRect>(rect, color); }
    void            EraseRect(const Rect& rect)                        { Post<ASViewFillRect>(rect, m_EraseColor); }

    void            FillCircle(const Point& position, float radius) { Post<ASViewFillCircle>(position, radius); }
    void            DrawString(const String& string) { Post<ASViewDrawString>(string); }

    virtual void    ScrollBy(const Point& offset)           { m_ScrollOffset += offset; UpdateScreenPos(); Post<ASViewScrollBy>(offset); }
    virtual void    ScrollBy(float vDeltaX, float vDeltaY) { ScrollBy(Point(vDeltaX, vDeltaY)); }
    virtual void    ScrollTo(Point topLeft)                { ScrollBy(topLeft - m_ScrollOffset); }
    virtual void    ScrollTo(float x, float y)             { ScrollTo(Point(x, y)); }
        
    Point           GetScrollOffset() const                { return m_ScrollOffset; }
    void            CopyRect(const Rect& srcRect, const Point& dstPos);
    void            DebugDraw(Color color, uint32_t drawFlags)         { Post<ASViewDebugDraw>(color, drawFlags); }
    
    //    void DrawBitmap( const Bitmap* pcBitmap, const Rect& cSrcRect, const Rect& cDstRect );
    void            DrawFrame(const Rect& rect, uint32_t styleFlags);
        
    FontHeight      GetFontHeight() const;
    float           GetStringWidth(const char* string, size_t length) const;
    float           GetStringWidth(const String& string) const { return GetStringWidth(string.data(), string.length()); }

    void            Flush();
    void            Sync();

    VFConnector<bool, MouseButton_e, const Point&> VFMouseDown;
    VFConnector<bool, MouseButton_e, const Point&> VFMouseUp;
    VFConnector<bool, const Point&>                VFMouseMoved;
    
    Signal<void>    SignalPreferredSizeChanged;
private:
    friend class Application;
    friend class ViewBase<View>;

    void Initialize();

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
            if (parent != nullptr && !parent->HasFlags(ViewFlags::WillDraw)) {
                newOffset = parent->m_PositionOffset + parent->m_Frame.TopLeft();
            } else {
                newOffset = Point(0.0f, 0.0f);
            }
        }            
        if (forceServerUpdate || newOffset != m_PositionOffset)
        {
            m_PositionOffset = newOffset;
            if (m_ServerHandle != INVALID_HANDLE/* && HasFlags(ViewFlags::WillDraw)*/)
            {
                Post<ASViewSetFrame>(m_Frame + m_PositionOffset, GetHandle());
//                GetApplication()->SetViewFrame(m_ServerHandle, m_Frame + m_PositionOffset);
            }                    
        }
        for (Ptr<View> child : m_ChildrenList) {
            child->UpdatePosition(false);
        }
    }

    void HandlePaint(const Rect& updateRect);
    
    void SetServerHandle(handler_id handle) { m_ServerHandle = handle; }
        
    void UpdateRingSize();
        
    handler_id m_ServerHandle = INVALID_HANDLE;
    
    Ptr<LayoutNode> m_LayoutNode;
    
    Rect         m_Borders = Rect(0.0f, 0.0f, 0.0f, 0.0f);
    float        m_Wheight = 1.0f;
    Alignment    m_HAlign = Alignment::Center;
    Alignment    m_VAlign = Alignment::Center;
    Point        m_LocalPrefSize[int(PrefSizeType::Count)];
    Point        m_PreferredSizes[int(PrefSizeType::Count)];

    float        m_WidthOverride[int(PrefSizeType::Count)]  = {0.0f, 0.0f};
    float        m_HeightOverride[int(PrefSizeType::Count)] = {0.0f, 0.0f};
    
    SizeOverride m_WidthOverrideType[int(PrefSizeType::Count)]  = {SizeOverride::None, SizeOverride::None};
    SizeOverride m_HeightOverrideType[int(PrefSizeType::Count)] = {SizeOverride::None, SizeOverride::None};
        
    bool         m_IsPrefSizeValid = false;
    bool         m_IsLayoutValid   = true;
    bool         m_DidScrollRect   = false;
    
    View*        m_WidthRing  = nullptr;
    View*        m_HeightRing = nullptr;
    
    Point        m_PositionOffset; // Offset relative to first parent that is not client only.
    
    int          m_BeginPainCount = 0;

    Ptr<Font> m_Font = ptr_new<Font>(kernel::GfxDriver::e_FontLarge);
    
    static WeakPtr<View> s_MouseDownView;
    
    ASPaintView::Receiver        RSPaintView;
    ASViewFrameChanged::Receiver RSViewFrameChanged;
    
    ASHandleMouseDown::Receiver  RSHandleMouseDown;
    ASHandleMouseUp::Receiver    RSHandleMouseUp;
    ASHandleMouseMove::Receiver  RSHandleMouseMove;
};

} // namespace
