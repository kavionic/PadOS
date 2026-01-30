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


class PSrvBitmap;

class PServerView : public PViewBase<PServerView>
{
public:
    PServerView(
        PSrvBitmap*          bitmap,
        const PString&      name,
        const PRect&         frame,
        const PPoint&        scrollOffset,
        PViewDockType        dockType,
        uint32_t            flags,
        int32_t             hideCount,
        PFocusKeyboardMode   focusKeyboardMode,
        PDrawingMode         drawingMode,
        float               penWidth,
        PFontID              fontID,
        PColor               eraseColor,
        PColor               bgColor,
        PColor               fgColor
    );

    virtual ~PServerView();
    void        SetClientHandle(port_id port, handler_id handle) { m_ClientPort = port; m_ClientHandle = handle; }
    port_id     GetClientPort() const   { return m_ClientPort; }
    handler_id  GetClientHandle() const { return m_ClientHandle; }

    PViewDockType   GetDockType() const { return m_DockType; }

    void SetIsWindowManagerControlled(bool value)   { m_IsWindowManagerControlled = value; }
    bool IsWindowManagerControlled() const          { return m_IsWindowManagerControlled; }

    void SetManagerHandle(handler_id handle)              { m_ManagerHandle = handle; }

    void HandleAddedToParent(Ptr<PServerView> parent, size_t index);
    void HandleRemovedFromParent(Ptr<PServerView> parent);

    bool        HandleMouseDown(PMouseButton button, const PPoint& position, const PMotionEvent& event);
    bool        HandleMouseUp(PMouseButton button, const PPoint& position, const PMotionEvent& event);
    bool        HandleMouseMove(PMouseButton button, const PPoint& position, const PMotionEvent& event);

    void        AddChild(Ptr<PServerView> child, size_t index);
    void        RemoveChild(Ptr<PServerView> child, bool removeAsHandler);
    void        RemoveThis(bool removeAsHandler);

    void        Show(bool show = true);
    bool        IsVisible() const;

    void        SetFrame(const PRect& frame, handler_id requestingClient);

    void        SetDrawRegion(Ptr<PRegion> pcReg);
    void        SetShapeRegion(Ptr<PRegion> pcReg);
    
    void        Invalidate(const PIRect& rect);
    void        Invalidate(bool reqursive = false);
    void        InvalidateNewAreas();
    
    void        MoveChilds();
    void        RebuildRegion();
    bool        ExcludeFromRegion(Ptr<PRegion> region, const PIPoint& offset);
    void        ClearDirtyRegFlags();
    void        UpdateRegions();
    void        DeleteRegions();
    Ptr<PRegion> GetRegion();
    
    void        ToggleDepth();
    void        BeginUpdate();
    void        EndUpdate();
    void        Paint(const PIRect& updateRect);
    
    void        RequestPaintIfNeeded();
    void        MarkModified(const PIRect& rect);
    void        SetDirtyRegFlags();

    void                SetFocusKeyboardMode(PFocusKeyboardMode mode);
    PFocusKeyboardMode   GetFocusKeyboardMode() const                 { return m_FocusKeyboardMode; }

    void        SetDrawingMode(PDrawingMode mode) { m_DrawingMode = mode; }
    void        SetEraseColor(PColor color) { m_EraseColor = color; }
    void        SetBgColor(PColor color)    { m_BgColor = color; }
    void        SetFgColor(PColor color)    { m_FgColor = color; }

    void        SetFont(int fontHandle) { m_Font->Set(PFontID(fontHandle)); }
    void        SetPenWidth(float width) { m_PenWidth = width; }

    void        MovePenTo(const PPoint& pos) { m_PenPosition = pos; }

    void        DrawLineTo(const PPoint& toPoint);
    void        DrawLine(const PPoint& fromPnt, const PPoint& toPnt);
    void        DrawThinLine(const PPoint& fromPnt, const PPoint& toPnt);
    void        DrawRect(const PRect& frame);        
    
    void        FillRect(const PRect& rect, PColor color);
    void        FillCircle(const PPoint& position, float radius);
    void        DrawString(const PString& string);
    void        CopyRect(const PRect& srcRect, const PPoint& dstPos);
    void        DrawBitmap(Ptr<PSrvBitmap> bitmap, const PRect& srcRect, const PPoint& dstPos);
    void        DrawScaledBitmap(Ptr<PSrvBitmap> bitmap, const PRect& srcRect, const PRect& dstRect);
    void        DebugDraw(PColor color, uint32_t drawFlags);
    void        ScrollBy( const PPoint& cDelta );

private:
    friend class PViewBase<PServerView>;

    void UpdateScreenPos();
    void DebugDrawRect(const PIRect& frame, PColor color);
        
    PSrvBitmap*  m_Bitmap        = nullptr;
    port_id     m_ClientPort    = INVALID_HANDLE;
    handler_id  m_ClientHandle  = INVALID_HANDLE;
    handler_id  m_ManagerHandle = INVALID_HANDLE;
    
    PViewDockType    m_DockType;

    PFocusKeyboardMode   m_FocusKeyboardMode = PFocusKeyboardMode::None;
    PDrawingMode         m_DrawingMode  = PDrawingMode::Copy;
    
    Ptr<PFont>   m_Font = ptr_new<PFont>(PFontID::e_FontLarge);
    
    PIPoint      m_DeltaMove;             // Relative movement since last region update
    PIPoint      m_DeltaSize;             // Relative sizing since last region update

    Ptr<PRegion> m_DrawConstrainReg;  // User specified "local" clipping list.
    Ptr<PRegion> m_ShapeConstrainReg; // User specified clipping list specifying the shape of the view.

    Ptr<PRegion> m_VisibleReg;        // Visible areas, not including non-transparent children
    Ptr<PRegion> m_FullReg;           // All visible areas, including children
    Ptr<PRegion> m_PrevVisibleReg;    // Temporary storage for m_VisibleReg during region rebuild
    Ptr<PRegion> m_PrevFullReg;       // Temporary storage for m_FullReg during region rebuild
    Ptr<PRegion> m_DrawReg;           // Only valid between BeginPaint()/EndPaint()
    Ptr<PRegion> m_DamageReg;         // Contains areas made visible since the last M_PAINT message sent
    Ptr<PRegion> m_ActiveDamageReg;
    
    bool    m_HasInvalidRegs = true; // True if something made our clipping region invalid
    bool    m_IsUpdating = false;    // True while we paint areas from the damage list
    bool    m_IsWindowManagerControlled = false;
};
