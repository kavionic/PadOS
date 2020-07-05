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


#pragma once

#include "GUI/View.h"

namespace os
{

class ServerView : public ViewBase<ServerView>
{
public:
    ServerView(const String& name, const Rect& frame, const Point& scrollOffset, uint32_t flags, int32_t hideCount, Color eraseColor, Color bgColor, Color fgColor);

    void SetClientHandle(port_id port, handler_id handle) { m_ClientPort = port; m_ClientHandle = handle; }
    void SetManagerHandle(handler_id handle)              { m_ManagerHandle = handle; }

    void HandleAddedToParent(Ptr<ServerView> parent) {}
    void HandleRemovedFromParent(Ptr<ServerView> parent) {}

    bool        HandleMouseDown(MouseButton_e button, const Point& position);
    bool        HandleMouseUp(MouseButton_e button, const Point& position);
    bool        HandleMouseMove(MouseButton_e button, const Point& position);

    void        AddChild(Ptr<ServerView> child, bool topmost = true);
    void        RemoveChild(Ptr<ServerView> child);
    void        RemoveThis();

    void        SetFrame(const Rect& frame, handler_id requestingClient);

    void        SetDrawRegion(Ptr<Region> pcReg);
    void        SetShapeRegion(Ptr<Region> pcReg);
    
    void        Invalidate(const IRect& rect);
    void        Invalidate(bool reqursive = false);
    void        InvalidateNewAreas();
    
    void        MoveChilds();
    void        SwapRegions(bool bForce);
    void        RebuildRegion(bool bForce);
    bool        ExcludeFromRegion(Ptr<Region> region, const IPoint& offset);
    void        ClearDirtyRegFlags();
    void        UpdateRegions(bool force = true, bool root = true);
    void        DeleteRegions();
    Ptr<Region> GetRegion();
    
    void        ToggleDepth();
    void        BeginUpdate();
    void        EndUpdate();
    void        Paint(const IRect& updateRect);
    
    void        UpdateIfNeeded(bool force);
    void        MarkModified(const IRect& rect);
    void        SetDirtyRegFlags();

    void        Show(bool visible = true);

    void        SetEraseColor(Color color) { m_EraseColor = color; m_EraseColor16 = color.GetColor16(); }
    void        SetBgColor(Color color)    { m_BgColor = color; m_BgColor16 = color.GetColor16(); }
    void        SetFgColor(Color color)    { m_FgColor = color; m_FgColor16 = color.GetColor16(); }

    void        SetFont(int fontHandle) { m_Font->Set(kernel::GfxDriver::Font_e(fontHandle)); }

    void        MovePenTo(const Point& pos) { m_PenPosition = pos; }

    void        DrawLineTo(const Point& toPoint);
    void        DrawLine(const Point& fromPnt, const Point& toPnt);
    void        DrawRect(const Rect& frame);        
    
    void        FillRect(const Rect& rect, Color color);
    void        FillCircle(const Point& position, float radius);
    void        DrawString(const String& string);
    void        CopyRect(const Rect& srcRect, const Point& dstPos);
    void        DebugDraw(Color color, uint32_t drawFlags);
    void        ScrollBy( const Point& cDelta );

private:
    friend class ViewBase<ServerView>;
        
    void DebugDrawRect(const IRect& frame);
        
    port_id     m_ClientPort = -1;
    handler_id  m_ClientHandle = -1;
    handler_id  m_ManagerHandle = -1;
    
    uint16_t    m_EraseColor16 = 0xffff;
    uint16_t    m_BgColor16    = 0xffff;
    uint16_t    m_FgColor16    = 0x0000;
    
    Ptr<Font>   m_Font = ptr_new<Font>(kernel::GfxDriver::e_FontLarge);
    
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
};

} // namespace