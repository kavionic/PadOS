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

class PScrollBar;
class PButtonGroup;
class PBitmap;


PColor pget_standard_color(PStandardColorID colorID);
void   pset_standard_color(PStandardColorID colorID, PColor color);


enum class PFocusKeyboardMode : uint8_t
{
    None,
    Alphanumeric,
    Numeric
};

class PView : public PViewBase<PView>
{
public:
    PView(const PString& name, Ptr<PView> parent = nullptr, uint32_t flags = 0);
    PView(PViewFactoryContext& context, Ptr<PView> parent, const pugi::xml_node& xmlData);
    PView(Ptr<PView> parent, handler_id serverHandle, const PString& name, const PRect& frame);
    virtual ~PView();

    // From EventHandler:
    virtual bool HandleMessage(int32_t code, const void* data, size_t length) override;

    PApplication* GetApplication() const;
    handler_id   GetServerHandle() const { return m_ServerHandle; }
    handler_id   GetParentServerHandle() const;

    virtual void OnAttachedToParent(Ptr<PView> parent) {}
    virtual void OnDetachedFromParent(Ptr<PView> parent) {}

    virtual void AttachedToScreen() {}
    virtual void AllAttachedToScreen() {}
    virtual void DetachedFromScreen() {}
    virtual void AllDetachedFromScreen() {}

    Ptr<PLayoutNode> GetLayoutNode() const;
    void            SetLayoutNode(Ptr<PLayoutNode> node);
    
    void            SetBorders(const PRect& border);
    void            SetBorders(float l, float t, float r, float b) { SetBorders(PRect(l, t, r, b)); }
    PRect            GetBorders() const;

    float           GetWheight() const;
    void            SetWheight(float wheight);
    
    void            SetHAlignment(PAlignment alignment);
    void            SetVAlignment(PAlignment alignment);
    PAlignment       GetHAlignment() const;
    PAlignment       GetVAlignment() const;
    
    void            SetWidthOverride(PPrefSizeType sizeType, PSizeOverride when, float size);
    void            SetHeightOverride(PPrefSizeType sizeType, PSizeOverride when, float size);
    
    void            AddToWidthRing(Ptr<PView> ring);
    void            RemoveFromWidthRing();

    void            AddToHeightRing(Ptr<PView> ring);
    void            RemoveFromHeightRing();
        
    void            InvalidateLayout();
    bool            RefreshLayout(int32_t maxIterations = 1, bool recursive = false);

//    virtual void Activated(bool isActive);
    //    virtual void WindowActivated( bool bIsActive );

    virtual void OnPaint(const PRect& updateRect) { EraseRect(updateRect); }

    virtual bool OnTouchDown(PMouseButton pointID, const PPoint& position, const PMotionEvent& motionEvent)  { return DispatchMouseDown(pointID, position, motionEvent); }
    virtual bool OnTouchUp(PMouseButton pointID, const PPoint& position, const PMotionEvent& motionEvent)    { return DispatchMouseUp(pointID, position, motionEvent);   }
    virtual bool OnTouchMove(PMouseButton pointID, const PPoint& position, const PMotionEvent& motionEvent)  { return DispatchMouseMove(pointID, position, motionEvent); }
    virtual bool OnLongPress(PMouseButton pointID, const PPoint& position, const PMotionEvent& motionEvent) { return false; }

    virtual bool OnMouseDown(PMouseButton button, const PPoint& position, const PMotionEvent& motionEvent);
    virtual bool OnMouseUp(PMouseButton button, const PPoint& position, const PMotionEvent& motionEvent);
    virtual bool OnMouseMove(PMouseButton button, const PPoint& position, const PMotionEvent& motionEvent);

    virtual void OnKeyDown(PKeyCodes keyCode, const PString& text, const PKeyEvent& keyEvent);
    virtual void OnKeyUp(PKeyCodes keyCode, const PString& text, const PKeyEvent& keyEvent);

    virtual void OnLayoutChanged();
    virtual void OnLayoutUpdated() {}
    virtual void OnFrameMoved(const PPoint& delta);
    virtual void OnFrameSized(const PPoint& delta);
    virtual void OnScreenFrameMoved(const PPoint& delta);
    virtual void OnViewScrolled(const PPoint& delta);
    virtual void OnFontChanged(Ptr<PFont> newFont);
    
    virtual void CalculatePreferredSize(PPoint* minSize, PPoint* maxSize, bool includeWidth, bool includeHeight);
    virtual PPoint CalculateContentSize() const;

    PPoint GetPreferredSize(PPrefSizeType sizeType) const;
    PPoint GetContentSize() const;

    void PreferredSizeChanged();
    void ContentSizeChanged();

//    virtual void WheelMoved( const Point& cDelta );

    void        AddChild(Ptr<PView> child);
    void        InsertChild(Ptr<PView> child, size_t index);
    void        RemoveChild(Ptr<PView> child);
    Ptr<PView>   RemoveChild(ChildList_t::iterator iterator);
    bool        RemoveThis();
    
    Ptr<PView>   GetChildAt(const PPoint& pos);
    template<typename T> Ptr<T> GetChildAt(const PPoint& pos) { return ptr_dynamic_cast<T>(GetChildAt(pos)); }

    Ptr<PView>   GetChildAt(size_t index);
    template<typename T> Ptr<T> GetChildAt(size_t index) { return ptr_dynamic_cast<T>(GetChildAt(index)); }

    Ptr<PScrollBar> GetVScrollBar() const;
    Ptr<PScrollBar> GetHScrollBar() const;

    void       Show(bool visible = true);
    void       Hide() { Show(false); }
    bool       IsVisible() const;
    bool       IsVisibleToMouse() const { return IsVisible() && !HasFlags(PViewFlags::IgnoreMouse); }
    virtual void MakeFocus(PMouseButton button, bool focus = true);
    virtual bool HasFocus(PMouseButton button) const;

    void SetKeyboardFocus(bool focus = true);
    bool HasKeyboardFocus() const;
    virtual void OnKeyboardFocusChanged(bool hasFocus) {}


    float  Width() const    { return m_Frame.Width(); }
    float  Height() const   { return m_Frame.Height(); }

    virtual void    SetFrame(const PRect& frame);
    void            MoveBy(const PPoint& delta)      { SetFrame(m_Frame + delta); }
    void            MoveBy(float x, float y)        { MoveBy(PPoint(x, y)); }
    void            MoveTo(const PPoint& position)   { SetFrame(m_Frame.Bounds() + position); }
    void            MoveTo(float x, float y)        { MoveTo(PPoint(x, y)); }

    virtual void    ResizeBy(const PPoint& delta);
    virtual void    ResizeBy(float deltaW, float deltaH);
    virtual void    ResizeTo(const PPoint& size);
    virtual void    ResizeTo(float w, float h);

    void                SetDrawingRegion(const PRegion& region);
    void                ClearDrawingRegion();
    void                SetShapeRegion(const PRegion& region);
    void                ClearShapeRegion();
    
    virtual void ToggleDepth() { Post<ASViewToggleDepth>(); }

    void                Invalidate(const PRect& rect);
    void                Invalidate();

    void                SetFocusKeyboardMode(PFocusKeyboardMode mode);
    PFocusKeyboardMode   GetFocusKeyboardMode() const { return m_FocusKeyboardMode; }

    void                SetDrawingMode(PDrawingMode mode);
    PDrawingMode         GetDrawingMode() const;
    void                SetFont(Ptr<PFont> font);
    Ptr<PFont>           GetFont() const;

    bool            SlotHandleMouseDown(PMouseButton button, const PPoint& position, const PMotionEvent& motionEvent);

    bool            HandleMouseDown(PMouseButton button, const PPoint& position, const PMotionEvent& motionEvent);
    void            HandleMouseUp(PMouseButton button, const PPoint& position, const PMotionEvent& motionEvent);
    void            HandleMouseMove(PMouseButton button, const PPoint& position, const PMotionEvent& motionEvent);
    
    void            SetFgColor(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha = 255)   { SetFgColor(PColor(red, green, blue, alpha)); }
    void            SetFgColor(PColor color)                                                     { if (color != m_FgColor) { m_FgColor = color; Post<ASViewSetFgColor>(color); } }
    void            SetFgColor(PStandardColorID colorID)                                         { SetFgColor(pget_standard_color(colorID)); }
    void            SetFgColor(PNamedColors colorID)                                             { SetFgColor(PColor(colorID)); }

    void            SetBgColor(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha = 255)   { SetBgColor(PColor(red, green, blue, alpha)); }
    void            SetBgColor(PColor color)                                                     { if (color != m_BgColor) { m_BgColor = color; Post<ASViewSetBgColor>(color); } }
    void            SetBgColor(PStandardColorID colorID)                                         { SetBgColor(pget_standard_color(colorID)); }
    void            SetBgColor(PNamedColors colorID)                                             { SetBgColor(PColor(colorID)); }

    void            SetEraseColor(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha = 255){ SetEraseColor(PColor(red, green, blue, alpha)); }
    void            SetEraseColor(PColor color)                                                  { if (color != m_EraseColor) { m_EraseColor = color; Post<ASViewSetEraseColor>(color); } }
    void            SetEraseColor(PStandardColorID colorID)                                      { SetEraseColor(pget_standard_color(colorID)); }
    void            SetEraseColor(PNamedColors colorID)                                          { SetEraseColor(PColor(colorID)); }

    void            SetPenWidth(float width)                           { if (width != m_PenWidth) { m_PenWidth = width; Post<ASViewSetPenWidth>(width); } }
    void            MovePenTo(const PPoint& pos)                        { m_PenPosition = pos; Post<ASViewMovePenTo>(pos); }
    void            MovePenTo(float x, float y)                        { MovePenTo(PPoint(x, y)); }
    void            MovePenBy(const PPoint& pos)                        { MovePenTo(m_PenPosition + pos); }
    void            MovePenBy(float x, float y)                        { MovePenBy(PPoint(x, y)); }
    PPoint           GetPenPosition() const                             { return m_PenPosition; }
    void            DrawLine(const PPoint& toPos)                       { Post<ASViewDrawLine1>(toPos); }
    void            DrawLine(const PPoint& fromPos, const PPoint& toPos) { Post<ASViewDrawLine2>(fromPos, toPos); }
    void            DrawLine(float x, float y)                         { DrawLine(PPoint(x, y)); }
    void            DrawLine(float x1, float y1, float x2, float y2)   { DrawLine(PPoint(x1, y1), PPoint(x2, y2)); }
    void            DrawRect(const PRect& frame)
    {
        MovePenTo(PPoint(frame.left, frame.top));
        DrawLine(PPoint(frame.right - 1.0f, frame.top));
        DrawLine(PPoint(frame.right - 1.0f, frame.bottom - 1.0f));
        DrawLine(PPoint(frame.left, frame.bottom - 1.0f));
        DrawLine(PPoint(frame.left, frame.top));
    }        
    void            FillRect(const PRect& rect)                         { Post<ASViewFillRect>(rect, m_FgColor); }
    void            FillRect(const PRect& rect, PColor color)            { Post<ASViewFillRect>(rect, color); }
    void            EraseRect(const PRect& rect)                        { Post<ASViewFillRect>(rect, m_EraseColor); }

    void            FillCircle(const PPoint& position, float radius) { Post<ASViewFillCircle>(position, radius); }
    void            DrawString(const PString& string) { Post<ASViewDrawString>(string); }
    void            DrawString(const PString& string, const PPoint& pos) { MovePenTo(pos); Post<ASViewDrawString>(string); }

    virtual void    ScrollBy(const PPoint& offset);
    virtual void    ScrollBy(float vDeltaX, float vDeltaY) { ScrollBy(PPoint(vDeltaX, vDeltaY)); }
    virtual void    ScrollTo(PPoint topLeft)                { ScrollBy(topLeft - m_ScrollOffset); }
    virtual void    ScrollTo(float x, float y)             { ScrollTo(PPoint(x, y)); }
        
    void            CopyRect(const PRect& srcRect, const PPoint& dstPos);
    void            DebugDraw(PColor color, uint32_t drawFlags)         { Post<ASViewDebugDraw>(color, drawFlags); }
    
    void            DrawBitmap(Ptr<const PBitmap> bitmap, const PRect& srcRect, const PPoint& dstPos);
    void            DrawBitmap(Ptr<const PBitmap> bitmap, const PRect& srcRect, const PRect& dstRect);
    void            DrawFrame(const PRect& rect, uint32_t styleFlags);
        
    PFontHeight      GetFontHeight() const;
    float           GetStringWidth(const char* string, size_t length) const;
    float           GetStringWidth(const PString& string) const { return GetStringWidth(string.data(), string.length()); }

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

    PPoint       ConvertToScreen(const PPoint& point) const   { return m_ScreenPos + point + m_ScrollOffset; }
    void        ConvertToScreen(PPoint* point) const         { *point += m_ScreenPos + m_ScrollOffset; }
    PRect        ConvertToScreen(const PRect& rect) const     { return rect + m_ScreenPos + m_ScrollOffset; }
    void        ConvertToScreen(PRect* rect) const           { *rect += m_ScreenPos + m_ScrollOffset; }
    PPoint       ConvertFromScreen(const PPoint& point) const { return point - m_ScreenPos - m_ScrollOffset; }
    void        ConvertFromScreen(PPoint* point) const       { *point -= m_ScreenPos + m_ScrollOffset; }
    PRect        ConvertFromScreen(const PRect& rect) const   { return rect - m_ScreenPos - m_ScrollOffset; }
    void        ConvertFromScreen(PRect* rect) const         { *rect -= m_ScreenPos + m_ScrollOffset; }

    VFConnector<bool (PView* view, PMouseButton pointID, const PPoint& position, const PMotionEvent& motionEvent)> VFTouchDown;
    VFConnector<bool (PView* view, PMouseButton pointID, const PPoint& position, const PMotionEvent& motionEvent)> VFTouchUp;
    VFConnector<bool (PView* view, PMouseButton pointID, const PPoint& position, const PMotionEvent& motionEvent)> VFTouchMove;
    VFConnector<bool (PView* view, PMouseButton pointID, const PPoint& position, const PMotionEvent& motionEvent)> VFLongPress;
    VFConnector<bool (PView* view, PMouseButton button , const PPoint& position, const PMotionEvent& motionEvent)> VFMouseDown;
    VFConnector<bool (PView* view, PMouseButton button , const PPoint& position, const PMotionEvent& motionEvent)> VFMouseUp;
    VFConnector<bool (PView* view, PMouseButton button , const PPoint& position, const PMotionEvent& motionEvent)> VFMouseMove;

    VFConnector<void (PView* view, PKeyCodes keyCode, const PString& text, const PKeyEvent& keyEvent)>               VFKeyDown;
    VFConnector<void (PView* view, PKeyCodes keyCode, const PString& text, const PKeyEvent& keyEvent)>               VFKeyUp;

    VFConnector<PPoint ()> VFCalculateContentSize;

    Signal<void (Ptr<PView> view)>                         SignalPreferredSizeChanged;
    Signal<void (PView* view)>                             SignalContentSizeChanged;
    Signal<void (const PPoint& deltaSize, Ptr<PView> view)> SignalFrameSized;
    Signal<void (const PPoint& deltaPos,  Ptr<PView> view)> SignalFrameMoved;
    Signal<void (const PPoint& offset,    Ptr<PView> view)> SignalViewScrolled;

private:
    friend class PApplication;
    friend class PViewBase<PView>;
    friend class PScrollBar;

    void Initialize();

    template<typename SIGNAL, typename... ARGS>
    void Post(ARGS&&... args)
    {
        if (m_ServerHandle != INVALID_HANDLE)
        {
            PApplication* app = GetApplication();
            if (app != nullptr)
            {
                assert(!app->IsRunning() || app->GetMutex().IsLocked());
                SIGNAL::Sender::Emit(app, &PApplication::AllocMessageBuffer, SIGNAL::GetID(), m_ServerHandle, args...);
            }
        }
    }        

    void HandleAddedToParent(Ptr<PView> parent, size_t index);
    void HandleRemovedFromParent(Ptr<PView> parent);

    void HandlePreAttachToScreen(PApplication* app);
    void HandleAttachedToScreen(PApplication* app);
    void HandleDetachedFromScreen();

    void HandlePaint(const PRect& updateRect);
    
    void SetServerHandle(handler_id handle) { m_ServerHandle = handle; }
        
    void UpdateRingSize();

    void SetVScrollBar(PScrollBar* scrollBar);
    void SetHScrollBar(PScrollBar* scrollBar);

    void SetFrameInternal(const PRect& frame, bool notifyServer);

    enum class UpdatePositionNotifyServer { Never, Always, IfChanged };
    void UpdatePosition(UpdatePositionNotifyServer notifyMode);

    bool DispatchTouchDown(PMouseButton pointID, const PPoint& position, const PMotionEvent& motionEvent);
    bool DispatchTouchUp(PMouseButton pointID, const PPoint& position, const PMotionEvent& motionEvent);
    bool DispatchTouchMove(PMouseButton pointID, const PPoint& position, const PMotionEvent& motionEvent);
    bool DispatchLongPress(PMouseButton pointID, const PPoint& position, const PMotionEvent& motionEvent);
    bool DispatchMouseDown(PMouseButton buttonID, const PPoint& position, const PMotionEvent& motionEvent);
    bool DispatchMouseUp(PMouseButton buttonID, const PPoint& position, const PMotionEvent& motionEvent);
    bool DispatchMouseMove(PMouseButton buttonID, const PPoint& position, const PMotionEvent& motionEvent);
    void DispatchKeyDown(PKeyCodes keyCode, const PString& text, const PKeyEvent& motionEvent);
    void DispatchKeyUp(PKeyCodes keyCode, const PString& text, const PKeyEvent& motionEvent);


    void SlotFrameChanged(const PRect& frame);
    void SlotKeyboardFocusChanged(bool hasFocus);

    handler_id          m_ServerHandle = INVALID_HANDLE;
    
    Ptr<PLayoutNode>     m_LayoutNode;
    
    PRect                m_Borders = PRect(0.0f, 0.0f, 0.0f, 0.0f);
    float               m_Wheight = 1.0f;
    PDrawingMode         m_DrawingMode = PDrawingMode::Overlay;
    PAlignment           m_HAlign = PAlignment::Center;
    PAlignment           m_VAlign = PAlignment::Center;
    PPoint               m_LocalPrefSize[int(PPrefSizeType::Count)];
    PPoint               m_PreferredSizes[int(PPrefSizeType::Count)];

    float               m_WidthOverride[int(PPrefSizeType::Count)]  = {0.0f, 0.0f};
    float               m_HeightOverride[int(PPrefSizeType::Count)] = {0.0f, 0.0f};
    
    PSizeOverride        m_WidthOverrideType[int(PPrefSizeType::Count)]  = {PSizeOverride::None, PSizeOverride::None};
    PSizeOverride        m_HeightOverrideType[int(PPrefSizeType::Count)] = {PSizeOverride::None, PSizeOverride::None};

    PFocusKeyboardMode   m_FocusKeyboardMode = PFocusKeyboardMode::None;
    bool                m_IsPrefSizeValid = false;
    bool                m_IsLayoutValid   = true;
    bool                m_IsLayoutPending = false;
    bool                m_DidScrollRect   = false;
    
    PView*               m_WidthRing  = nullptr;
    PView*               m_HeightRing = nullptr;
    
    PPoint               m_PositionOffset; // Offset relative to first parent that is not client only.
    int                 m_BeginPainCount = 0;

    PScrollBar*          m_HScrollBar = nullptr;
    PScrollBar*          m_VScrollBar = nullptr;

    Ptr<PFont>           m_Font = ptr_new<PFont>(PFontID::e_FontLarge);
    
    ASPaintView         RSPaintView;
    ASViewFrameChanged  RSViewFrameChanged;
    ASViewFocusChanged  RSViewFocusChanged;

    ASHandleMouseDown   RSHandleMouseDown;
    ASHandleMouseUp     RSHandleMouseUp;
    ASHandleMouseMove   RSHandleMouseMove;
};
