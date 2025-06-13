// This file is part of PadOS.
//
// Copyright (C) 1999-2025 Kurt Skauen <http://kavionic.com/>
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


enum class FocusKeyboardMode : uint8_t
{
    None,
    Alphanumeric,
    Numeric
};

class View : public ViewBase<View>
{
public:
    View(const String& name, Ptr<View> parent = nullptr, uint32_t flags = 0);
    View(ViewFactoryContext& context, Ptr<View> parent, const pugi::xml_node& xmlData);
    View(Ptr<View> parent, handler_id serverHandle, const String& name, const Rect& frame);
    virtual ~View();

    // From EventHandler:
    virtual bool HandleMessage(int32_t code, const void* data, size_t length) override;

    Application* GetApplication() const;
    handler_id   GetServerHandle() const { return m_ServerHandle; }
    handler_id   GetParentServerHandle() const;

    virtual void OnAttachedToParent(Ptr<View> parent) {}
    virtual void OnDetachedFromParent(Ptr<View> parent) {}

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
    bool            RefreshLayout(int32_t maxIterations = 1, bool recursive = false);

//    virtual void Activated(bool isActive);
    //    virtual void WindowActivated( bool bIsActive );

    virtual void OnPaint(const Rect& updateRect) { EraseRect(updateRect); }

    virtual bool OnTouchDown(MouseButton_e pointID, const Point& position, const MotionEvent& motionEvent)  { return DispatchMouseDown(pointID, position, motionEvent); }
    virtual bool OnTouchUp(MouseButton_e pointID, const Point& position, const MotionEvent& motionEvent)    { return DispatchMouseUp(pointID, position, motionEvent);   }
    virtual bool OnTouchMove(MouseButton_e pointID, const Point& position, const MotionEvent& motionEvent)  { return DispatchMouseMove(pointID, position, motionEvent); }
    virtual bool OnLongPress(MouseButton_e pointID, const Point& position, const MotionEvent& motionEvent) { return false; }

    virtual bool OnMouseDown(MouseButton_e button, const Point& position, const MotionEvent& motionEvent);
    virtual bool OnMouseUp(MouseButton_e button, const Point& position, const MotionEvent& motionEvent);
    virtual bool OnMouseMove(MouseButton_e button, const Point& position, const MotionEvent& motionEvent);

    virtual void OnKeyDown(KeyCodes keyCode, const String& text, const KeyEvent& keyEvent);
    virtual void OnKeyUp(KeyCodes keyCode, const String& text, const KeyEvent& keyEvent);

    virtual void OnLayoutChanged();
    virtual void OnFrameMoved(const Point& delta);
    virtual void OnFrameSized(const Point& delta);
    virtual void OnScreenFrameMoved(const Point& delta);
    virtual void OnViewScrolled(const Point& delta);
    virtual void OnFontChanged(Ptr<Font> newFont);
    
    virtual void CalculatePreferredSize(Point* minSize, Point* maxSize, bool includeWidth, bool includeHeight);
    virtual Point CalculateContentSize() const;

    Point GetPreferredSize(PrefSizeType sizeType) const;
    Point GetContentSize() const;

    void PreferredSizeChanged();
    void ContentSizeChanged();

//    virtual void WheelMoved( const Point& cDelta );

    void        AddChild(Ptr<View> child);
    void        InsertChild(Ptr<View> child, size_t index);
    void        RemoveChild(Ptr<View> child);
    Ptr<View>   RemoveChild(ChildList_t::iterator iterator);
    bool        RemoveThis();
    
    Ptr<View>   GetChildAt(const Point& pos);
    template<typename T> Ptr<T> GetChildAt(const Point& pos) { return ptr_dynamic_cast<T>(GetChildAt(pos)); }

    Ptr<View>   GetChildAt(size_t index);
    template<typename T> Ptr<T> GetChildAt(size_t index) { return ptr_dynamic_cast<T>(GetChildAt(index)); }

    Ptr<ScrollBar> GetVScrollBar() const;
    Ptr<ScrollBar> GetHScrollBar() const;

    void       Show(bool visible = true);
    void       Hide() { Show(false); }
    bool       IsVisible() const;
    bool       IsVisibleToMouse() const { return IsVisible() && !HasFlags(ViewFlags::IgnoreMouse); }
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

    void                Invalidate(const Rect& rect);
    void                Invalidate();

    void                SetFocusKeyboardMode(FocusKeyboardMode mode);
    FocusKeyboardMode   GetFocusKeyboardMode() const { return m_FocusKeyboardMode; }

    void                SetDrawingMode(DrawingMode mode);
    DrawingMode         GetDrawingMode() const;
    void                SetFont(Ptr<Font> font);
    Ptr<Font>           GetFont() const;

    bool            SlotHandleMouseDown(MouseButton_e button, const Point& position, const MotionEvent& motionEvent);

    bool            HandleMouseDown(MouseButton_e button, const Point& position, const MotionEvent& motionEvent);
    void            HandleMouseUp(MouseButton_e button, const Point& position, const MotionEvent& motionEvent);
    void            HandleMouseMove(MouseButton_e button, const Point& position, const MotionEvent& motionEvent);
    
    void            SetFgColor(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha = 255)   { SetFgColor(Color(red, green, blue, alpha)); }
    void            SetFgColor(Color color)                                                     { if (color != m_FgColor) { m_FgColor = color; Post<ASViewSetFgColor>(color); } }
    void            SetFgColor(StandardColorID colorID)                                         { SetFgColor(get_standard_color(colorID)); }
    void            SetFgColor(NamedColors colorID)                                             { SetFgColor(Color(colorID)); }

    void            SetBgColor(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha = 255)   { SetBgColor(Color(red, green, blue, alpha)); }
    void            SetBgColor(Color color)                                                     { if (color != m_BgColor) { m_BgColor = color; Post<ASViewSetBgColor>(color); } }
    void            SetBgColor(StandardColorID colorID)                                         { SetBgColor(get_standard_color(colorID)); }
    void            SetBgColor(NamedColors colorID)                                             { SetBgColor(Color(colorID)); }

    void            SetEraseColor(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha = 255){ SetEraseColor(Color(red, green, blue, alpha)); }
    void            SetEraseColor(Color color)                                                  { if (color != m_EraseColor) { m_EraseColor = color; Post<ASViewSetEraseColor>(color); } }
    void            SetEraseColor(StandardColorID colorID)                                      { SetEraseColor(get_standard_color(colorID)); }
    void            SetEraseColor(NamedColors colorID)                                          { SetEraseColor(Color(colorID)); }

    void            SetPenWidth(float width)                           { if (width != m_PenWidth) { m_PenWidth = width; Post<ASViewSetPenWidth>(width); } }
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
    void            DrawBitmap(Ptr<const Bitmap> bitmap, const Rect& srcRect, const Rect& dstRect);
    void            DrawFrame(const Rect& rect, uint32_t styleFlags);
        
    FontHeight      GetFontHeight() const;
    float           GetStringWidth(const char* string, size_t length) const;
    float           GetStringWidth(const String& string) const { return GetStringWidth(string.data(), string.length()); }

    void            Flush();
    void            Sync();

//    Point       ConvertToRoot(const Point& point) const     { return m_ScreenPos + point + m_ScrollOffset; }
//    void        ConvertToRoot(Point* point) const           { *point += m_ScreenPos + m_ScrollOffset; }
//    Rect        ConvertToRoot(const Rect& rect) const       { return rect + m_ScreenPos + m_ScrollOffset; }
//    void        ConvertToRoot(Rect* rect) const             { *rect += m_ScreenPos + m_ScrollOffset; }
//    Point       ConvertFromRoot(const Point& point) const   { return point - m_ScreenPos - m_ScrollOffset; }
//    void        ConvertFromRoot(Point* point) const         { *point -= m_ScreenPos + m_ScrollOffset; }
//    Rect        ConvertFromRoot(const Rect& rect) const     { return rect - m_ScreenPos - m_ScrollOffset; }
//    void        ConvertFromRoot(Rect* rect) const           { *rect -= m_ScreenPos + m_ScrollOffset; }

    Point       ConvertToScreen(const Point& point) const   { return m_ScreenPos + point + m_ScrollOffset; }
    void        ConvertToScreen(Point* point) const         { *point += m_ScreenPos + m_ScrollOffset; }
    Rect        ConvertToScreen(const Rect& rect) const     { return rect + m_ScreenPos + m_ScrollOffset; }
    void        ConvertToScreen(Rect* rect) const           { *rect += m_ScreenPos + m_ScrollOffset; }
    Point       ConvertFromScreen(const Point& point) const { return point - m_ScreenPos - m_ScrollOffset; }
    void        ConvertFromScreen(Point* point) const       { *point -= m_ScreenPos + m_ScrollOffset; }
    Rect        ConvertFromScreen(const Rect& rect) const   { return rect - m_ScreenPos - m_ScrollOffset; }
    void        ConvertFromScreen(Rect* rect) const         { *rect -= m_ScreenPos + m_ScrollOffset; }

    VFConnector<bool (View* view, MouseButton_e pointID, const Point& position, const MotionEvent& motionEvent)> VFTouchDown;
    VFConnector<bool (View* view, MouseButton_e pointID, const Point& position, const MotionEvent& motionEvent)> VFTouchUp;
    VFConnector<bool (View* view, MouseButton_e pointID, const Point& position, const MotionEvent& motionEvent)> VFTouchMove;
    VFConnector<bool (View* view, MouseButton_e pointID, const Point& position, const MotionEvent& motionEvent)> VFLongPress;
    VFConnector<bool (View* view, MouseButton_e button , const Point& position, const MotionEvent& motionEvent)> VFMouseDown;
    VFConnector<bool (View* view, MouseButton_e button , const Point& position, const MotionEvent& motionEvent)> VFMouseUp;
    VFConnector<bool (View* view, MouseButton_e button , const Point& position, const MotionEvent& motionEvent)> VFMouseMove;

    VFConnector<void (View* view, KeyCodes keyCode, const String& text, const KeyEvent& keyEvent)>               VFKeyDown;
    VFConnector<void (View* view, KeyCodes keyCode, const String& text, const KeyEvent& keyEvent)>               VFKeyUp;

    VFConnector<Point ()> VFCalculateContentSize;

    Signal<void (Ptr<View> view)>                         SignalPreferredSizeChanged;
    Signal<void (View* view)>                             SignalContentSizeChanged;
    Signal<void (const Point& deltaSize, Ptr<View> view)> SignalFrameSized;
    Signal<void (const Point& deltaPos,  Ptr<View> view)> SignalFrameMoved;
    Signal<void (const Point& offset,    Ptr<View> view)> SignalViewScrolled;

private:
    friend class Application;
    friend class ViewBase<View>;
    friend class ScrollBar;

    void Initialize();

    template<typename SIGNAL, typename... ARGS>
    void Post(ARGS&&... args)
    {
        if (m_ServerHandle != INVALID_HANDLE)
        {
            Application* app = GetApplication();
            if (app != nullptr)
            {
                assert(!app->IsRunning() || app->GetMutex().IsLocked());
                SIGNAL::Sender::Emit(app, &Application::AllocMessageBuffer, SIGNAL::GetID(), m_ServerHandle, args...);
            }
        }
    }        

    void HandleAddedToParent(Ptr<View> parent, size_t index);
    void HandleRemovedFromParent(Ptr<View> parent);

    void HandlePreAttachToScreen(Application* app);
    void HandleAttachedToScreen(Application* app);
    void HandleDetachedFromScreen();

    void HandlePaint(const Rect& updateRect);
    
    void SetServerHandle(handler_id handle) { m_ServerHandle = handle; }
        
    void UpdateRingSize();

    void SetVScrollBar(ScrollBar* scrollBar);
    void SetHScrollBar(ScrollBar* scrollBar);

    void SetFrameInternal(const Rect& frame, bool notifyServer);

    enum class UpdatePositionNotifyServer { Never, Always, IfChanged };
    void UpdatePosition(UpdatePositionNotifyServer notifyMode);

    bool DispatchTouchDown(MouseButton_e pointID, const Point& position, const MotionEvent& motionEvent);
    bool DispatchTouchUp(MouseButton_e pointID, const Point& position, const MotionEvent& motionEvent);
    bool DispatchTouchMove(MouseButton_e pointID, const Point& position, const MotionEvent& motionEvent);
    bool DispatchLongPress(MouseButton_e pointID, const Point& position, const MotionEvent& motionEvent);
    bool DispatchMouseDown(MouseButton_e buttonID, const Point& position, const MotionEvent& motionEvent);
    bool DispatchMouseUp(MouseButton_e buttonID, const Point& position, const MotionEvent& motionEvent);
    bool DispatchMouseMove(MouseButton_e buttonID, const Point& position, const MotionEvent& motionEvent);
    void DispatchKeyDown(KeyCodes keyCode, const String& text, const KeyEvent& motionEvent);
    void DispatchKeyUp(KeyCodes keyCode, const String& text, const KeyEvent& motionEvent);


    void SlotFrameChanged(const Rect& frame);
    void SlotKeyboardFocusChanged(bool hasFocus);

    handler_id          m_ServerHandle = INVALID_HANDLE;
    
    Ptr<LayoutNode>     m_LayoutNode;
    
    Rect                m_Borders = Rect(0.0f, 0.0f, 0.0f, 0.0f);
    float               m_Wheight = 1.0f;
    DrawingMode         m_DrawingMode = DrawingMode::Overlay;
    Alignment           m_HAlign = Alignment::Center;
    Alignment           m_VAlign = Alignment::Center;
    Point               m_LocalPrefSize[int(PrefSizeType::Count)];
    Point               m_PreferredSizes[int(PrefSizeType::Count)];

    float               m_WidthOverride[int(PrefSizeType::Count)]  = {0.0f, 0.0f};
    float               m_HeightOverride[int(PrefSizeType::Count)] = {0.0f, 0.0f};
    
    SizeOverride        m_WidthOverrideType[int(PrefSizeType::Count)]  = {SizeOverride::None, SizeOverride::None};
    SizeOverride        m_HeightOverrideType[int(PrefSizeType::Count)] = {SizeOverride::None, SizeOverride::None};

    FocusKeyboardMode   m_FocusKeyboardMode = FocusKeyboardMode::None;
    bool                m_IsPrefSizeValid = false;
    bool                m_IsLayoutValid   = true;
    bool                m_IsLayoutPending = false;
    bool                m_DidScrollRect   = false;
    
    View*               m_WidthRing  = nullptr;
    View*               m_HeightRing = nullptr;
    
    Point               m_PositionOffset; // Offset relative to first parent that is not client only.
    int                 m_BeginPainCount = 0;

    ScrollBar*          m_HScrollBar = nullptr;
    ScrollBar*          m_VScrollBar = nullptr;

    Ptr<Font>           m_Font = ptr_new<Font>(Font_e::e_FontLarge);
    
    ASPaintView         RSPaintView;
    ASViewFrameChanged  RSViewFrameChanged;
    ASViewFocusChanged  RSViewFocusChanged;

    ASHandleMouseDown   RSHandleMouseDown;
    ASHandleMouseUp     RSHandleMouseUp;
    ASHandleMouseMove   RSHandleMouseMove;
};

} // namespace
