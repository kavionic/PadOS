// This file is part of PadOS.
//
// Copyright (C) 2018-2021 Kurt Skauen <http://kavionic.com/>
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


#pragma once

#include <GUI/View.h>

namespace os
{

class SrvBitmap;

class ServerView : public ViewBase<ServerView>
{
public:
    ServerView(SrvBitmap* bitmap, const String& name, const Rect& frame, const Point& scrollOffset, ViewDockType dockType, uint32_t flags, int32_t hideCount, FocusKeyboardMode focusKeyboardMode, DrawingMode drawingMode, Font_e fontID, Color eraseColor, Color bgColor, Color fgColor);
    virtual ~ServerView();
    void        SetClientHandle(port_id port, handler_id handle) { m_ClientPort = port; m_ClientHandle = handle; }
    port_id     GetClientPort() const   { return m_ClientPort; }
    handler_id  GetClientHandle() const { return m_ClientHandle; }

    ViewDockType   GetDockType() const { return m_DockType; }

    void SetIsWindowManagerControlled(bool value)   { m_IsWindowManagerControlled = value; }
    bool IsWindowManagerControlled() const          { return m_IsWindowManagerControlled; }

    void SetManagerHandle(handler_id handle)              { m_ManagerHandle = handle; }

    void HandleAddedToParent(Ptr<ServerView> parent, size_t index);
    void HandleRemovedFromParent(Ptr<ServerView> parent);

    bool        HandleMouseDown(MouseButton_e button, const Point& position, const MotionEvent& event);
    bool        HandleMouseUp(MouseButton_e button, const Point& position, const MotionEvent& event);
    bool        HandleMouseMove(MouseButton_e button, const Point& position, const MotionEvent& event);

    void        AddChild(Ptr<ServerView> child, size_t index);
    void        RemoveChild(Ptr<ServerView> child, bool removeAsHandler);
    void        RemoveThis(bool removeAsHandler);

    void        Show(bool show = true);
    bool        IsVisible() const;

    void        SetFrame(const Rect& frame, handler_id requestingClient);

    void        SetDrawRegion(Ptr<Region> pcReg);
    void        SetShapeRegion(Ptr<Region> pcReg);
    
    void        Invalidate(const IRect& rect);
    void        Invalidate(bool reqursive = false);
    void        InvalidateNewAreas();
    
    void        MoveChilds();
    void        RebuildRegion();
    bool        ExcludeFromRegion(Ptr<Region> region, const IPoint& offset);
    void        ClearDirtyRegFlags();
    void        UpdateRegions();
    void        DeleteRegions();
    Ptr<Region> GetRegion();
    
    void        ToggleDepth();
    void        BeginUpdate();
    void        EndUpdate();
    void        Paint(const IRect& updateRect);
    
    void        RequestPaintIfNeeded();
    void        MarkModified(const IRect& rect);
    void        SetDirtyRegFlags();

    void                SetFocusKeyboardMode(FocusKeyboardMode mode);
    FocusKeyboardMode   GetFocusKeyboardMode() const                 { return m_FocusKeyboardMode; }

    void        SetDrawingMode(DrawingMode mode) { m_DrawingMode = mode; }
    void        SetEraseColor(Color color) { m_EraseColor = color; }
    void        SetBgColor(Color color)    { m_BgColor = color; }
    void        SetFgColor(Color color)    { m_FgColor = color; }

    void        SetFont(int fontHandle) { m_Font->Set(Font_e(fontHandle)); }

    void        MovePenTo(const Point& pos) { m_PenPosition = pos; }

    void        DrawLineTo(const Point& toPoint);
    void        DrawLine(const Point& fromPnt, const Point& toPnt);
    void        DrawRect(const Rect& frame);        
    
    void        FillRect(const Rect& rect, Color color);
    void        FillCircle(const Point& position, float radius);
    void        DrawString(const String& string);
    void        CopyRect(const Rect& srcRect, const Point& dstPos);
    void        DrawBitmap(Ptr<SrvBitmap> bitmap, const Rect& srcRect, const Point& dstPos);
    void        DebugDraw(Color color, uint32_t drawFlags);
    void        ScrollBy( const Point& cDelta );

private:
    friend class ViewBase<ServerView>;

    void UpdateScreenPos();
    void DebugDrawRect(const IRect& frame, Color color);
        
    SrvBitmap*  m_Bitmap        = nullptr;
    port_id     m_ClientPort    = INVALID_HANDLE;
    handler_id  m_ClientHandle  = INVALID_HANDLE;
    handler_id  m_ManagerHandle = INVALID_HANDLE;
    
    ViewDockType    m_DockType;

    FocusKeyboardMode   m_FocusKeyboardMode = FocusKeyboardMode::None;
    DrawingMode         m_DrawingMode  = DrawingMode::Copy;
    
    Ptr<Font>   m_Font = ptr_new<Font>(Font_e::e_FontLarge);
    
    IPoint      m_DeltaMove;             // Relative movement since last region update
    IPoint      m_DeltaSize;             // Relative sizing since last region update

    Ptr<Region> m_DrawConstrainReg;  // User specified "local" clipping list.
    Ptr<Region> m_ShapeConstrainReg; // User specified clipping list specifying the shape of the view.

    Ptr<Region> m_VisibleReg;        // Visible areas, not including non-transparent children
    Ptr<Region> m_FullReg;           // All visible areas, including children
    Ptr<Region> m_PrevVisibleReg;    // Temporary storage for m_VisibleReg during region rebuild
    Ptr<Region> m_PrevFullReg;       // Temporary storage for m_FullReg during region rebuild
    Ptr<Region> m_DrawReg;           // Only valid between BeginPaint()/EndPaint()
    Ptr<Region> m_DamageReg;         // Contains areas made visible since the last M_PAINT message sent
    Ptr<Region> m_ActiveDamageReg;
    
    bool    m_HasInvalidRegs = true; // True if something made our clipping region invalid
    bool    m_IsUpdating = false;    // True while we paint areas from the damage list
    bool    m_IsWindowManagerControlled = false;
};

} // namespace
