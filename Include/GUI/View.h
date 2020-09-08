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
#include "ViewFactoryContext.h"

namespace pugi
{
class xml_node;
}

#define DEFINE_FLAG_MAP_ENTRY(NAMESPACE, FLAG) {#FLAG, NAMESPACE::FLAG}

namespace os
{

class ButtonGroup;
class ScrollBar;
class Bitmap;

Color get_standard_color(StandardColorID colorID);
void  set_standard_color(StandardColorID colorID, Color color);


class View : public ViewBase<View>
{
public:
    View(const String& name, Ptr<View> parent = nullptr, uint32_t flags = 0);
    View(ViewFactoryContext* context, Ptr<View> parent, const pugi::xml_node& xmlData);
    View(Ptr<View> parent, handler_id serverHandle, const String& name, const Rect& frame);
    virtual ~View();

    // From EventHandler:
    virtual bool HandleMessage(int32_t code, const void* data, size_t length) override;

    Application* GetApplication() const;
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

    virtual bool OnTouchDown(MouseButton_e pointID, const Point& position, const MotionEvent& event)  { return OnMouseDown(pointID, position, event); }
    virtual bool OnTouchUp(MouseButton_e pointID, const Point& position, const MotionEvent& event)    { return OnMouseUp(pointID, position, event);   }
    virtual bool OnTouchMove(MouseButton_e pointID, const Point& position, const MotionEvent& event)  { return OnMouseMove(pointID, position, event); }

    virtual bool OnMouseDown(MouseButton_e button, const Point& position, const MotionEvent& event);
    virtual bool OnMouseUp(MouseButton_e button, const Point& position, const MotionEvent& event);
    virtual bool OnMouseMove(MouseButton_e button, const Point& position, const MotionEvent& event);

    virtual void OnKeyDown(KeyCodes keyCode, const String& text, const KeyEvent& event);
    virtual void OnKeyUp(KeyCodes keyCode, const String& text, const KeyEvent& event);

    virtual void  FrameMoved(const Point& delta);
    virtual void  FrameSized(const Point& delta);
    virtual void  ScreenFrameMoved(const Point& delta);
    virtual void  ViewScrolled(const Point& delta);
    virtual void  FontChanged(Ptr<Font> newFont);
    
    virtual void CalculatePreferredSize(Point* minSize, Point* maxSize, bool includeWidth, bool includeHeight) const;
    
    Point GetPreferredSize(PrefSizeType sizeType) const;
    virtual Point GetContentSize() const;

    void PreferredSizeChanged();
    void ContentSizeChanged();


//    virtual void WheelMoved( const Point& cDelta );

    void        AddChild(Ptr<View> child);
    void        RemoveChild(Ptr<View> child);
    Ptr<View>   RemoveChild(ChildList_t::iterator iterator);
    bool        RemoveThis();
    
    Ptr<View>   GetChildAt(const Point& pos);
    Ptr<View>   GetChildAt(size_t index);

    Ptr<ScrollBar> GetVScrollBar() const;
    Ptr<ScrollBar> GetHScrollBar() const;

    void       Show(bool visible = true);
    void       Hide() { Show(false); }
    bool       IsVisible() const;
    virtual void MakeFocus(MouseButton_e button, bool focus = true);
    virtual bool HasFocus(MouseButton_e button) const;

    void SetKeyboardFocus(bool focus = true);
    bool HasKeyboardFocus() const;
    virtual void OnKeyboardFocusChanged(bool hasFocus) {}


    float  Width() const    { return m_Frame.Width(); }
    float  Height() const   { return m_Frame.Height(); }

    virtual void    SetFrame(const Rect& frame);
    void            MoveBy(const Point& delta)      { SetFrame(m_Frame + delta); }
    void            MoveBy(float x, float y)        { MoveBy(Point(x, y)); }
    void            MoveTo(const Point& position)   { SetFrame(m_Frame.Bounds() + position); }
    void            MoveTo(float x, float y)        { MoveTo(Point(x, y)); }

    virtual void    ResizeBy(const Point& delta);
    virtual void    ResizeBy(float deltaW, float deltaH);
    virtual void    ResizeTo(const Point& size);
    virtual void    ResizeTo(float w, float h);

    void                SetDrawingRegion(const Region& region);
    void                ClearDrawingRegion();
    void                SetShapeRegion(const Region& region);
    void                ClearShapeRegion();
    
    virtual void ToggleDepth() { Post<ASViewToggleDepth>(); }

    void                Invalidate(const Rect& rect, bool recurse = false);
    void                Invalidate(bool recurse = false);

    void                SetDrawingMode(DrawingMode mode);
    DrawingMode         GetDrawingMode() const;
    void                SetFont(Ptr<Font> font);
    Ptr<Font>           GetFont() const;

    bool            SlotHandleMouseDown(MouseButton_e button, const Point& position, const MotionEvent& event);

    bool            HandleMouseDown(MouseButton_e button, const Point& position, const MotionEvent& event);
    void            HandleMouseUp(MouseButton_e button, const Point& position, const MotionEvent& event);
    void            HandleMouseMove(MouseButton_e button, const Point& position, const MotionEvent& event);
    
    void            SetFgColor(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha = 255)   { SetFgColor(Color(red, green, blue, alpha)); }
    void            SetFgColor(Color color)                                                     { m_FgColor = color; Post<ASViewSetFgColor>(color); }
    void            SetFgColor(StandardColorID colorID)                                         { SetFgColor(get_standard_color(colorID)); }
    void            SetFgColor(NamedColors colorID)                                             { SetFgColor(Color(colorID)); }

    void            SetBgColor(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha = 255)   { SetBgColor(Color(red, green, blue, alpha)); }
    void            SetBgColor(Color color)                                                     { m_BgColor = color; Post<ASViewSetBgColor>(color); }
    void            SetBgColor(StandardColorID colorID)                                         { SetBgColor(get_standard_color(colorID)); }
    void            SetBgColor(NamedColors colorID)                                             { SetBgColor(Color(colorID)); }

    void            SetEraseColor(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha = 255){ SetEraseColor(Color(red, green, blue, alpha)); }
    void            SetEraseColor(Color color)                                                  { m_EraseColor = color; Post<ASViewSetEraseColor>(color); }
    void            SetEraseColor(StandardColorID colorID)                                      { SetEraseColor(get_standard_color(colorID)); }
    void            SetEraseColor(NamedColors colorID)                                          { SetEraseColor(Color(colorID)); }

    void            MovePenTo(const Point& pos)                        { m_PenPosition = pos; Post<ASViewMovePenTo>(pos); }
    void            MovePenTo(float x, float y)                        { MovePenTo(Point(x, y)); }
    void            MovePenBy(const Point& pos)                        { MovePenTo(m_PenPosition + pos); }
    void            MovePenBy(float x, float y)                        { MovePenBy(Point(x, y)); }
    Point           GetPenPosition() const                             { return m_PenPosition; }
    void            DrawLine(const Point& toPos)                       { Post<ASViewDrawLine1>(toPos); }
    void            DrawLine(const Point& fromPos, const Point& toPos) { Post<ASViewDrawLine2>(fromPos, toPos); }
    void            DrawLine(float x, float y)                         { DrawLine(Point(x, y)); }
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
    void            DrawString(const String& string, const Point& pos) { MovePenTo(pos); Post<ASViewDrawString>(string); }

    virtual void    ScrollBy(const Point& offset);
    virtual void    ScrollBy(float vDeltaX, float vDeltaY) { ScrollBy(Point(vDeltaX, vDeltaY)); }
    virtual void    ScrollTo(Point topLeft)                { ScrollBy(topLeft - m_ScrollOffset); }
    virtual void    ScrollTo(float x, float y)             { ScrollTo(Point(x, y)); }
        
    void            CopyRect(const Rect& srcRect, const Point& dstPos);
    void            DebugDraw(Color color, uint32_t drawFlags)         { Post<ASViewDebugDraw>(color, drawFlags); }
    
    void            DrawBitmap(Ptr<const Bitmap> bitmap, const Rect& srcRect, const Point& dstPos);
    void            DrawFrame(const Rect& rect, uint32_t styleFlags);
        
    FontHeight      GetFontHeight() const;
    float           GetStringWidth(const char* string, size_t length) const;
    float           GetStringWidth(const String& string) const { return GetStringWidth(string.data(), string.length()); }

    void            Flush();
    void            Sync();

    Point       ConvertToRoot(const Point& point) const     { return m_ScreenPos + point + m_ScrollOffset; }
    void        ConvertToRoot(Point* point) const           { *point += m_ScreenPos + m_ScrollOffset; }
    Rect        ConvertToRoot(const Rect& rect) const       { return rect + m_ScreenPos + m_ScrollOffset; }
    void        ConvertToRoot(Rect* rect) const             { *rect += m_ScreenPos + m_ScrollOffset; }
    Point       ConvertFromRoot(const Point& point) const   { return point - m_ScreenPos - m_ScrollOffset; }
    void        ConvertFromRoot(Point* point) const         { *point -= m_ScreenPos + m_ScrollOffset; }
    Rect        ConvertFromRoot(const Rect& rect) const     { return rect - m_ScreenPos - m_ScrollOffset; }
    void        ConvertFromRoot(Rect* rect) const           { *rect -= m_ScreenPos + m_ScrollOffset; }

    Point       ConvertToScreen(const Point& point) const   { return m_ScreenPos + point + m_ScrollOffset; }
    void        ConvertToScreen(Point* point) const         { *point += m_ScreenPos + m_ScrollOffset; }
    Rect        ConvertToScreen(const Rect& rect) const     { return rect + m_ScreenPos + m_ScrollOffset; }
    void        ConvertToScreen(Rect* rect) const           { *rect += m_ScreenPos + m_ScrollOffset; }
    Point       ConvertFromScreen(const Point& point) const { return point - m_ScreenPos - m_ScrollOffset; }
    void        ConvertFromScreen(Point* point) const       { *point -= m_ScreenPos + m_ScrollOffset; }
    Rect        ConvertFromScreen(const Rect& rect) const   { return rect - m_ScreenPos - m_ScrollOffset; }
    void        ConvertFromScreen(Rect* rect) const         { *rect -= m_ScreenPos + m_ScrollOffset; }

    VFConnector<bool, MouseButton_e, const Point&, const MotionEvent&> VFMouseDown;
    VFConnector<bool, MouseButton_e, const Point&, const MotionEvent&> VFMouseUp;
    VFConnector<bool, const Point&, const MotionEvent&>                VFMouseMoved;
    
    Signal<void, Ptr<View>>               SignalPreferredSizeChanged;
    Signal<void, View*>                   SignalContentSizeChanged;
    Signal<void, const Point&, Ptr<View>> SignalFrameSized;
    Signal<void, const Point&, Ptr<View>> SignalFrameMoved;
    Signal<void, const Point&, Ptr<View>> SignalViewScrolled;
private:
    friend class Application;
    friend class ViewBase<View>;
    friend class ScrollBar;

    void Initialize();

    template<typename SIGNAL, typename... ARGS>
    void Post(ARGS&&... args)
    {
        Application* app = GetApplication();
        if (app != nullptr) {
            SIGNAL::Sender::Emit(app, &Application::AllocMessageBuffer, m_ServerHandle, args...);
        }
    }        

    void HandleAddedToParent(Ptr<View> parent);
    void HandleRemovedFromParent(Ptr<View> parent);

    void HandleDetachedFromScreen();

    void HandlePaint(const Rect& updateRect);
    
    void SetServerHandle(handler_id handle) { m_ServerHandle = handle; }
        
    void UpdateRingSize();

    void SetVScrollBar(ScrollBar* scrollBar);
    void SetHScrollBar(ScrollBar* scrollBar);

    void UpdatePosition(bool forceServerUpdate);

    void SlotKeyboardFocusChanged(bool hasFocus);

    handler_id      m_ServerHandle = INVALID_HANDLE;
    
    Ptr<LayoutNode> m_LayoutNode;
    
    Rect            m_Borders = Rect(0.0f, 0.0f, 0.0f, 0.0f);
    float           m_Wheight = 1.0f;
    DrawingMode     m_DrawingMode = DrawingMode::Overlay;
    Alignment       m_HAlign = Alignment::Center;
    Alignment       m_VAlign = Alignment::Center;
    Point           m_LocalPrefSize[int(PrefSizeType::Count)];
    Point           m_PreferredSizes[int(PrefSizeType::Count)];

    float           m_WidthOverride[int(PrefSizeType::Count)]  = {0.0f, 0.0f};
    float           m_HeightOverride[int(PrefSizeType::Count)] = {0.0f, 0.0f};
    
    SizeOverride    m_WidthOverrideType[int(PrefSizeType::Count)]  = {SizeOverride::None, SizeOverride::None};
    SizeOverride    m_HeightOverrideType[int(PrefSizeType::Count)] = {SizeOverride::None, SizeOverride::None};
        
    bool            m_IsPrefSizeValid = false;
    bool            m_IsLayoutValid   = true;
    bool            m_DidScrollRect   = false;
    
    View*           m_WidthRing  = nullptr;
    View*           m_HeightRing = nullptr;
    
    Point           m_PositionOffset; // Offset relative to first parent that is not client only.
    
    int             m_BeginPainCount = 0;

    ScrollBar*      m_HScrollBar = nullptr;
    ScrollBar*      m_VScrollBar = nullptr;

    Ptr<Font>       m_Font = ptr_new<Font>(Font_e::e_FontLarge);
    
    ASPaintView::Receiver        RSPaintView;
    ASViewFrameChanged::Receiver RSViewFrameChanged;
    ASViewFocusChanged::Receiver RSViewFocusChanged;

    ASHandleMouseDown::Receiver  RSHandleMouseDown;
    ASHandleMouseUp::Receiver    RSHandleMouseUp;
    ASHandleMouseMove::Receiver  RSHandleMouseMove;
};

} // namespace
