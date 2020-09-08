// This file is part of PadOS.
//
// Copyright (C) 2018-2020 Kurt Skauen <http://kavionic.com/>
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

#include <System/Platform.h>

#include <string.h>

#include <ApplicationServer/ApplicationServer.h>
#include <ApplicationServer/DisplayDriver.h>
#include <ApplicationServer/ServerBitmap.h>
#include <Utils/Utils.h>

#include "ServerView.h"

using namespace os;
using namespace kernel;

static int g_ServerViewCount = 0;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ServerView::ServerView(SrvBitmap* bitmap, const String& name, const Rect& frame, const Point& scrollOffset, ViewDockType dockType, uint32_t flags, int32_t hideCount, DrawingMode drawingMode, Color eraseColor, Color bgColor, Color fgColor)
    : ViewBase(name, frame, scrollOffset, flags, hideCount, eraseColor, bgColor, fgColor)
    , m_Bitmap(bitmap)
    , m_DockType(dockType)
    , m_DrawingMode(drawingMode)
{
    g_ServerViewCount++;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ServerView::~ServerView()
{
    g_ServerViewCount--;

	while (!m_ChildrenList.empty())	{
		RemoveChild(m_ChildrenList[m_ChildrenList.size() - 1], true);
	}
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerView::HandleRemovedFromParent(Ptr<ServerView> parent)
{
    ApplicationServer* server = static_cast<ApplicationServer*>(GetLooper());
    if (server != nullptr) {
        server->ViewDestructed(this);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

    
bool ServerView::HandleMouseDown(MouseButton_e button, const Point& position, const MotionEvent& event)
{
    if (m_ClientHandle != INVALID_HANDLE && !HasFlags(ViewFlags::IgnoreMouse))
    {
        if (m_ManagerHandle != INVALID_HANDLE)
        {
            if (!ASHandleMouseDown::Sender::Emit(get_window_manager_port(), m_ManagerHandle, TimeValMicros::infinit, button, position, event))
            {
                printf("ERROR: ServerView::HandleMouseDown() failed to send message: %s\n", strerror(get_last_error()));
                return false;
            }            
        }
                    
        if (!ASHandleMouseDown::Sender::Emit(m_ClientPort, m_ClientHandle, TimeValMicros::zero, button, position, event)) {
            printf("ERROR: ServerView::HandleMouseDown() failed to send message: %s\n", strerror(get_last_error()));
            return false;
        }
        ApplicationServer* server = static_cast<ApplicationServer*>(GetLooper());
        if (server !=  nullptr) {
            server->SetMouseDownView(button, ptr_tmp_cast(this));
        }
        return true;
    }
    
    for (Ptr<ServerView> child : reverse_ranged(m_ChildrenList))
    {
        if (child->m_Frame.DoIntersect(position))
        {
            Point childPos = position - child->m_Frame.TopLeft() - child->m_ScrollOffset;
            if (child->HandleMouseDown(button, childPos, event))
            {
                return true;
            }
        }
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool ServerView::HandleMouseUp(MouseButton_e button, const Point& position, const MotionEvent& event)
{
    if (m_ClientHandle != -1 )
    {
        if (!ASHandleMouseUp::Sender::Emit(m_ClientPort, m_ClientHandle, TimeValMicros::zero, button, position, event)) {
            printf("ERROR: ServerView::HandleMouseUp() failed to send message: %s\n", strerror(get_last_error()));
        }
        return true;
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool ServerView::HandleMouseMove(MouseButton_e button, const Point& position, const MotionEvent& event)
{
    if (m_ClientHandle != -1 ) {
        if (!ASHandleMouseMove::Sender::Emit(m_ClientPort, m_ClientHandle, TimeValMicros::zero, button, position, event)) {
            printf("ERROR: ServerView::HandleMouseMove() failed to send message: %s\n", strerror(get_last_error()));
        }
        return true;
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerView::AddChild(Ptr<ServerView> child, bool topmost)
{
    LinkChild(child, topmost);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerView::RemoveChild(Ptr<ServerView> child, bool removeAsHandler)
{
    UnlinkChild(child);

    if (removeAsHandler)
    {
        Looper* looper = child->GetLooper();
        if (looper != nullptr) {
            looper->RemoveHandler(child);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerView::RemoveThis(bool removeAsHandler)
{
    Ptr<ServerView> parent = m_Parent.Lock();
    if (parent != nullptr)
    {
        parent->RemoveChild(ptr_tmp_cast(this), removeAsHandler);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerView::SetFrame(const Rect& rect, handler_id requestingClient)
{
    IRect intFrame(rect);

    IRect prevIFrame = GetIFrame();
    m_Frame = rect;
    UpdateScreenPos();

    if (intFrame == prevIFrame) {
        return;
    }

    Ptr<ServerView> parent = m_Parent.Lock();

    if (m_HideCount == 0)
    {
        m_DeltaMove += IPoint(intFrame.left, intFrame.top ) - IPoint(prevIFrame.left, prevIFrame.top);
        m_DeltaSize += IPoint(intFrame.Width(), intFrame.Height() ) - IPoint(prevIFrame.Width(), prevIFrame.Height());

        if (parent != nullptr)
        {
            for (Ptr<ServerView> i = parent; i != nullptr; i = i->GetParent())
            {
                if ((i->m_Flags & ViewFlags::Transparent) == 0) {
                    i->SetDirtyRegFlags();
                    break;
                }                    
            }

            auto i = parent->GetChildRIterator(ptr_tmp_cast(this));
            if (i != parent->m_ChildrenList.rend())
            {
                for (++i; i != parent->m_ChildrenList.rend(); ++i)
                {
                    Ptr<ServerView> sibling = *i;
		    IRect siblingIFrame = sibling->GetIFrame();
                    if (siblingIFrame.DoIntersect(prevIFrame) || siblingIFrame.DoIntersect(intFrame))
                    {
                        sibling->MarkModified(prevIFrame - siblingIFrame.TopLeft());
                        sibling->MarkModified(intFrame - siblingIFrame.TopLeft());
                    }
                }                
            }
        }            
    }
//    m_IFrame = intFrame;
    
    if (requestingClient != -1)
    {
        if (requestingClient == m_ClientHandle)
        {
            if (m_ManagerHandle != -1)
            {
                ApplicationServer* server = static_cast<ApplicationServer*>(GetLooper());
                if (server !=  nullptr) {
                    ASViewFrameChanged::Sender::Emit(get_window_manager_port(), m_ManagerHandle, TimeValMicros::infinit, m_Frame);
                }                    
            }
        }
        else
        {
            if (!ASViewFrameChanged::Sender::Emit(m_ClientPort, m_ClientHandle, TimeValMicros::zero, m_Frame)) {
                printf("ERROR: ServerView::SetFrame() failed to send message: %s\n", strerror(get_last_error()));
            }            
        }
    }
    
    if (parent == nullptr) {
        Invalidate();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerView::SetDrawRegion(Ptr<Region> region)
{
    m_DrawConstrainReg = region;

    if (m_HideCount == 0) {
        m_HasInvalidRegs = true;
    }
    m_DrawReg = nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerView::SetShapeRegion(Ptr<Region> region)
{
    m_ShapeConstrainReg = region;

    if (m_HideCount == 0)
    {
        Ptr<ServerView> parent = m_Parent.Lock();
        if ( parent != nullptr )
        {
            for (Ptr<ServerView> i = parent; i != nullptr; i = i->GetParent())
            {
                if ((i->m_Flags & ViewFlags::Transparent) == 0) {
                    i->SetDirtyRegFlags();
                    break;
                }                    
            }
            
            auto i = parent->GetChildRIterator(ptr_tmp_cast(this));
            if (i != parent->m_ChildrenList.rend())
            {
		IRect intFrame = GetIFrame();
                for (++i; i != parent->m_ChildrenList.rend(); ++i)
                {
                    Ptr<ServerView> sibling = *i;
		    IRect siblingIFrame = sibling->GetIFrame();
                    if ( siblingIFrame.DoIntersect(intFrame) ) {
                        sibling->MarkModified(intFrame - siblingIFrame.TopLeft());
                    }
                }
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerView::Invalidate(const IRect& rect)
{
    if (m_HideCount == 0)
    {
        if (m_DamageReg == nullptr) {
            m_DamageReg = ptr_new<Region>(rect);
        } else {
            m_DamageReg->Include(rect);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerView::Invalidate(bool reqursive)
{
    if (m_HideCount == 0)
    {
        m_DamageReg = ptr_new<Region>(static_cast<IRect>(GetNormalizedBounds()));
        if (reqursive)
        {
            for (Ptr<ServerView> child : m_ChildrenList) {
                child->Invalidate(true);
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerView::InvalidateNewAreas()
{
    if ( m_HideCount > 0 ) {
        return;
    }
    /*
    if (m_pcWindow != nullptr && (m_pcWindow->GetDesktopMask() & (1 << get_active_desktop())) == 0 ) {
        return;
    }*/
  
    if (m_HasInvalidRegs)
    {
        if ( ((m_Flags & ViewFlags::FullUpdateOnResizeH) && m_DeltaSize.x != 0) || ((m_Flags & ViewFlags::FullUpdateOnResizeV) && m_DeltaSize.y != 0) )
        {
            Invalidate(false);
        }
        else
        {
            if (m_VisibleReg != nullptr)
            {
                try
                {
                    Ptr<Region> region = ptr_new<Region>(*m_VisibleReg);
    
                    if (m_PrevVisibleReg != nullptr) {
                        region->Exclude( *m_PrevVisibleReg );
                    }
                    if (m_DamageReg == nullptr)
                    {
                        if (!region->IsEmpty()) {
                            m_DamageReg = region;
                        }
                    }
                    else
                    {
                        for (const IRect& clip : region->m_Rects) {
                            Invalidate(clip);
                        }
                    }
                }
                catch(const std::bad_alloc&) {}
            }
        }
        m_PrevVisibleReg = nullptr;

        m_DeltaSize = IPoint( 0, 0 );
        m_DeltaMove = IPoint( 0, 0 );
    }
  
    for (Ptr<ServerView> child : m_ChildrenList) {
        child->InvalidateNewAreas();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

struct BlitSortCompare
{
    BlitSortCompare(const IPoint& deltaMove) : m_DeltaMove(deltaMove) {}
        
    bool operator()(IRect* lhs, IRect* rhs) const
    {
        if (lhs->left >= rhs->right && lhs->right <= rhs->left)
        {
            if (m_DeltaMove.x < 0) {
                return lhs->left < rhs->left;
            } else {
                return rhs->left < lhs->left;
            }
        }
        else
        {
            if (m_DeltaMove.y < 0) {
                return lhs->top < rhs->top;
            } else {
                return rhs->top < lhs->top;
            }
        }
    }
    IPoint m_DeltaMove;
};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerView::MoveChilds()
{
    if ( m_HideCount > 0  /*|| nullptr == m_pcBitmap */) {
        return;
    }
/*    if (m_pcWindow != nullptr && (m_pcWindow->GetDesktopMask() & (1 << get_active_desktop())) == 0) {
        return;
    }*/
  
    if ( m_HasInvalidRegs )
    {
        IRect bounds(GetNormalizedBounds());
        IPoint screenPos(m_ScreenPos);
        for (Ptr<ServerView> child : m_ChildrenList)
        {
            if (child->m_DeltaMove.x == 0 && child->m_DeltaMove.y == 0) {
                continue;
            }
            if (child->m_FullReg == nullptr || child->m_PrevFullReg == nullptr) {
                continue;
            }

            Ptr<Region> region = ptr_new<Region>(*child->m_PrevFullReg);
            region->Intersect(*child->m_FullReg);

            if (region->IsEmpty()) {
                continue;
            }

            IPoint cChildOffset = IPoint(child->m_ScreenPos);
            IPoint cChildMove(child->m_DeltaMove);
            for (IRect& clip : region->m_Rects)
            {
                // Transform into parents coordinate system
                clip += cChildOffset;
                assert(clip.IsValid());
            }

            std::vector<IRect*> clipList;
            clipList.reserve(region->m_Rects.size());

            for (IRect& clip : region->m_Rects) {
                clipList.push_back(&clip);
            }
            std::sort(clipList.begin(), clipList.end(), BlitSortCompare(cChildMove));

            for (const IRect* clip : clipList) {
                m_Bitmap->m_Driver->CopyRect(m_Bitmap, m_Bitmap, m_BgColor, m_FgColor, *clip - cChildMove, clip->TopLeft(), DrawingMode::Copy);
            }
        }

        // Since the parent window is shrinked before the children is moved
        // we may need to redraw the right and bottom edges.

        Ptr<ServerView> parent = m_Parent.Lock();
        if ( parent != nullptr && (m_DeltaMove.x != 0 || m_DeltaMove.y != 0) )
        {
            if (parent->m_DeltaSize.x < 0)
            {
                IRect rect = bounds;

		IRect parentIFrame = parent->GetIFrame();

                rect.left = rect.right + int(parent->m_DeltaSize.x + parentIFrame.right - parentIFrame.left - GetIFrame().right);

                if (rect.IsValid()) {
                    Invalidate( rect );
                }
            }
            if (parent->m_DeltaSize.y < 0)
            {
                IRect rect = bounds;

		IRect parentIFrame = parent->GetIFrame();
		rect.top = rect.bottom + int(parent->m_DeltaSize.y + parentIFrame.bottom - parentIFrame.top - GetIFrame().bottom);

                if (rect.IsValid()) {
                    Invalidate(rect);
                }
            }
        }
        m_PrevFullReg = nullptr;
    }
    for (Ptr<ServerView> child : m_ChildrenList) {
        child->MoveChilds();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// DESC:
///     Stores the previous visible region in m_PrevVisibleReg and then
///     rebuilds m_VisibleReg, starting with whatever is left of our parent
///     and then removing areas covered by siblings.
/// NOTE:
///     Areas covered by children are not removed.
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerView::SwapRegions( bool bForce )
{
    if ( bForce ) {
        m_HasInvalidRegs = true;
    }
  
    for (Ptr<ServerView> child : m_ChildrenList) {
        child->SwapRegions( bForce );
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerView::RebuildRegion(bool bForce)
{
    if (m_HideCount > 0)
    {
        if (m_VisibleReg != nullptr) {
            DeleteRegions();
        }
        return;
    }

    if (bForce) {
        m_HasInvalidRegs = true;
    }

    if (m_HasInvalidRegs)
    {
        m_DrawReg = nullptr;

        assert(m_PrevVisibleReg == nullptr);
        assert(m_PrevFullReg == nullptr);

        m_PrevVisibleReg = m_VisibleReg;
        m_PrevFullReg = m_FullReg;

        IPoint scrollOffset(m_ScrollOffset);

        Ptr<ServerView> parent = m_Parent.Lock();
        if (parent == nullptr)
        {
            m_FullReg = ptr_new<Region>(GetIFrame());
        }
        else
        {
            const IRect intFrame = GetIFrame();
            assert(parent->m_FullReg != nullptr);
            if (parent->m_FullReg == nullptr) {
                m_FullReg = ptr_new<Region>(intFrame.Bounds());
            } else {
                m_FullReg = ptr_new<Region>(*parent->m_FullReg, intFrame + IPoint(parent->m_ScrollOffset), true);
            }
            if (m_ShapeConstrainReg != nullptr) {
                m_FullReg->Intersect(*m_ShapeConstrainReg);
            }
            IPoint topLeft(intFrame.TopLeft());
            auto i = parent->GetChildIterator(ptr_tmp_cast(this));
            if (i != parent->m_ChildrenList.end())
            {
                for (++i; i != parent->m_ChildrenList.end(); ++i) // Loop over all siblings above us.
                {
                    Ptr<ServerView> sibling = *i;
                    if (sibling->m_HideCount == 0)
                    {
                        if (sibling->GetIFrame().DoIntersect(intFrame))
                        {
                            sibling->ExcludeFromRegion(m_FullReg, -topLeft);
                        }
                    }
                }
            }
            m_FullReg->Optimize();
        }
        m_VisibleReg = ptr_new<Region>(*m_FullReg);

        if ((m_Flags & ViewFlags::DrawOnChildren) == 0)
        {
            bool regModified = false;
//            IPoint scrollOffset(m_ScrollOffset);
            for (Ptr<ServerView> child : m_ChildrenList)
            {
                // Remove children from child region
                if (child->ExcludeFromRegion(m_VisibleReg, scrollOffset)) {
                    regModified = true;
                }
            }
            if (regModified) {
                m_VisibleReg->Optimize();
            }
        }
    }
    for (Ptr<ServerView> child : m_ChildrenList) {
        child->RebuildRegion(bForce);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool ServerView::ExcludeFromRegion(Ptr<Region> region, const IPoint& offset)
{
    if (m_HideCount == 0)
    {
        if ((m_Flags & ViewFlags::Transparent) == 0)
        {
            if ( m_ShapeConstrainReg == nullptr ) {
                region->Exclude(GetIFrame() + offset);
            } else {
                region->Exclude(*m_ShapeConstrainReg, GetITopLeft() + offset);
            }
            return true;
        }
        else
        {
            bool wasModified = false;
            IPoint framePos = GetITopLeft();
            IPoint scrollOffset(m_ScrollOffset);
            for (Ptr<ServerView> child : m_ChildrenList)
            {
                if (child->ExcludeFromRegion(region, offset + framePos + scrollOffset)) {
                    wasModified = true;
                }                    
            }                
            return wasModified;
        }
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerView::ClearDirtyRegFlags()
{
    m_HasInvalidRegs = false;
    for (Ptr<ServerView> child : m_ChildrenList) {
        child->ClearDirtyRegFlags();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerView::UpdateRegions(bool bForce, bool bRoot)
{
    RebuildRegion( bForce );
    MoveChilds();
    InvalidateNewAreas();

    ApplicationServer* server = static_cast<ApplicationServer*>(GetLooper());
    if (server !=  nullptr)
    {
        if (m_HasInvalidRegs && server->GetTopView() == this /*m_pcBitmap != nullptr*/ && m_DamageReg != nullptr)
        {
            if ( m_Bitmap == ApplicationServer::GetScreenBitmap())
            {
                Region cDrawReg(*m_VisibleReg);
                cDrawReg.Intersect(*m_DamageReg);
        
                IPoint screenPos(m_ScreenPos);
                for (const IRect& clip : cDrawReg.m_Rects) {
                    m_Bitmap->m_Driver->FillRect(m_Bitmap, clip + screenPos, m_EraseColor);
                }
            }
            m_DamageReg = nullptr;
        }
    }        
    UpdateIfNeeded(false);
    ClearDirtyRegFlags();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerView::DeleteRegions()
{
    assert(m_PrevVisibleReg == nullptr);
    assert(m_PrevFullReg == nullptr);

    m_VisibleReg      = nullptr;
    m_FullReg         = nullptr;
    m_DrawReg         = nullptr;
    m_DamageReg       = nullptr;
    m_ActiveDamageReg = nullptr;
    
    for (Ptr<ServerView> child : m_ChildrenList) {
        child->DeleteRegions();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<Region> ServerView::GetRegion()
{
    if ( m_HideCount > 0 ) {
        return nullptr;
    }
    if (m_IsUpdating && m_ActiveDamageReg == nullptr) {
        return nullptr;
    }

    if ( !m_IsUpdating )
    {
        if ( m_DrawConstrainReg == nullptr )
        {
            return m_VisibleReg;
        }
        else
        {
            if ( m_DrawReg == nullptr )
            {
                m_DrawReg = ptr_new<Region>(*m_VisibleReg);
                m_DrawReg->Intersect( *m_DrawConstrainReg );
            }
        }
    }
    else if (m_DrawReg == nullptr && m_VisibleReg != nullptr)
    {
        m_DrawReg = ptr_new<Region>(*m_VisibleReg);

        assert(m_ActiveDamageReg != nullptr);
        m_DrawReg->Intersect(*m_ActiveDamageReg);
        if (m_DrawConstrainReg != nullptr) {
            m_DrawReg->Intersect( *m_DrawConstrainReg );
        }
        m_DrawReg->Optimize();
    }
    assert(m_DrawReg != nullptr);
    return m_DrawReg;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerView::ToggleDepth()
{
    Ptr<ServerView> self = ptr_tmp_cast(this);
    Ptr<ServerView> parent = GetParent();
    if ( parent != NULL )
    {
        if (parent->m_ChildrenList[parent->m_ChildrenList.size()-1] == self)
        {
            parent->RemoveChild(self, false);
            parent->AddChild(self, false);
        }
        else
        {
            parent->RemoveChild(self, false);
            parent->AddChild(self, true);
        }

        Ptr<ServerView> opacParent = GetOpacParent(parent, nullptr);
        
        opacParent->SetDirtyRegFlags();
    
	IRect intFrame = GetIFrame();
	for (Ptr<ServerView> sibling : *parent)
        {
	    IRect siblingIFrame = sibling->GetIFrame();
            if (siblingIFrame.DoIntersect(intFrame)) {
                sibling->MarkModified(intFrame - siblingIFrame.TopLeft());
            }
        }
        opacParent->UpdateRegions(false);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerView::BeginUpdate()
{
    if (m_VisibleReg != nullptr) {
        m_IsUpdating = true;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerView::EndUpdate( void )
{
    m_ActiveDamageReg = nullptr;
    m_DrawReg         = nullptr;
    
    m_IsUpdating = false;
    
    if (m_DamageReg != nullptr)
    {
        m_ActiveDamageReg = m_DamageReg;
        m_DamageReg = nullptr;
        Paint(static_cast<Rect>(m_ActiveDamageReg->GetBounds()));
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerView::Paint(const IRect& updateRect)
{
    if ( m_HideCount > 0 || m_IsUpdating == true || m_ClientHandle == -1) {
        return;
    }
    if (!ASPaintView::Sender::Emit(m_ClientPort, m_ClientHandle, TimeValMicros::zero, updateRect - IPoint(m_ScrollOffset))) {
        printf("ERROR: ServerView::Paint() failed to send message: %s\n", strerror(get_last_error()));        
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerView::UpdateIfNeeded(bool force)
{
    if ( m_HideCount > 0) {
        return;
    }
    //    if ( m_pcWindow != nullptr && ((m_pcWindow->GetDesktopMask() & (1 << get_active_desktop())) == 0 ||
    //                              m_pcWindow->HasPendingSizeEvents( this )) ) {
    //      return;
    //    }
    
    if (m_DamageReg != nullptr)
    {
        if (m_ActiveDamageReg == nullptr)
        {
            m_ActiveDamageReg = m_DamageReg;
            m_DamageReg = nullptr;
            m_ActiveDamageReg->Optimize();
            Paint( static_cast<Rect>(m_ActiveDamageReg->GetBounds()));
        }
    }
    for (Ptr<ServerView> child : m_ChildrenList) {
        child->UpdateIfNeeded(force);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerView::MarkModified(const IRect& rect)
{
    if (GetNormalizedBounds().DoIntersect(rect))
    {
        m_HasInvalidRegs = true;
        IPoint scrollOffset(m_ScrollOffset);
        
        for (Ptr<ServerView> child : m_ChildrenList) {
            child->MarkModified(rect - child->GetITopLeft() - scrollOffset);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerView::SetDirtyRegFlags()
{
    m_HasInvalidRegs = true;
    
    for (Ptr<ServerView> child : m_ChildrenList) {
        child->SetDirtyRegFlags();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerView::Show(bool doShow)
{
    //    if ( m_pcParent == nullptr || m_pcWindow == nullptr ) {
    //      dbprintf( "Error: Layer::Show() attempt to hide root layer\n" );
    //      return;
    //    }
    if ( doShow ) {
        m_HideCount--;
    } else {
        m_HideCount++;
    }

    Ptr<ServerView> parent = m_Parent.Lock();
    if (parent != nullptr)
    {
        for (Ptr<ServerView> i = parent; i != nullptr; i = i->GetParent())
        {
            if ((i->m_Flags & ViewFlags::Transparent) == 0) {
                i->SetDirtyRegFlags();
                break;
            }                    
        }
	IRect intFrame = GetIFrame();
        for (Ptr<ServerView> sibling : parent->m_ChildrenList)
        {
	    IRect siblingIFrame = sibling->GetIFrame();
            if (siblingIFrame.DoIntersect(intFrame)) {
                sibling->MarkModified(intFrame - siblingIFrame.TopLeft());
            }
        }
    }
    for (auto child = m_ChildrenList.rbegin(); child != m_ChildrenList.rend(); ++child) {
        (*child)->Show(doShow);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerView::DrawLineTo(const Point& toPoint)
{
    DrawLine(m_PenPosition, toPoint);
    m_PenPosition = toPoint;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerView::DrawLine(const Point& fromPnt, const Point& toPnt )
{
    Ptr<const Region> region = GetRegion();
    if (region != nullptr)
    {
        IPoint screenPos(m_ScreenPos);
        IPoint fromPntScr(fromPnt + m_ScrollOffset);
        IPoint toPntScr(toPnt + m_ScrollOffset);

        IRect boundingBox;
        if (fromPntScr.x > toPntScr.x) {
            std::swap(toPntScr, fromPntScr);
        }
        boundingBox.left  = fromPntScr.x;
        boundingBox.right = toPntScr.x + 1;
        
        if (fromPntScr.y < toPntScr.y) {
            boundingBox.top    = fromPntScr.y;
            boundingBox.bottom = toPntScr.y + 1;
        } else {
            boundingBox.top  = toPntScr.y;
            boundingBox.bottom = fromPntScr.y + 1;
        }

        fromPntScr += screenPos;
        toPntScr   += screenPos;
        
        IRect screenFrame = ApplicationServer::GetScreenIFrame();

        if (!Region::ClipLine(screenFrame, &fromPntScr, &toPntScr)) return;
        
        for (const IRect& clip : region->m_Rects)
        {
            if (clip.DoIntersect(boundingBox))
            {
                m_Bitmap->m_Driver->DrawLine(m_Bitmap, clip + screenPos, fromPntScr, toPntScr, m_FgColor, m_DrawingMode);
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerView::DrawRect(const Rect& frame)
{
    DrawLine(Point(frame.left, frame.top), Point(frame.right - 1.0f, frame.top));
    DrawLine(Point(frame.right - 1.0f, frame.top), Point(frame.right - 1.0f, frame.bottom - 1.0f));
    DrawLine(Point(frame.right - 1.0f, frame.bottom - 1.0f), Point(frame.left, frame.bottom - 1.0f));
    DrawLine(Point(frame.left, frame.bottom - 1.0f), Point(frame.left, frame.top));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerView::FillRect(const Rect& rect, Color color)
{
    Ptr<const Region> region = GetRegion();
    if (region != nullptr)
    {
        IPoint screenPos(m_ScreenPos);
        IRect  rectScr(rect + m_ScrollOffset);
        
        for (const IRect& clip : region->m_Rects)
        {
            IRect clippedRect(rectScr & clip);
            if (clippedRect.IsValid()) {
                clippedRect += screenPos;
                m_Bitmap->m_Driver->FillRect(m_Bitmap, clippedRect, color);
            }
        }
    }        
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerView::FillCircle(const Point& position, float radius)
{
    if (int(position.y + m_ScreenPos.y + m_ScrollOffset.y + radius) >= 510) return; // Broken clipping past that.
    
    Ptr<const Region> region = GetRegion();
    if (region != nullptr)
    {
        IPoint screenPos(m_ScreenPos);
        IPoint positionScr(position + m_ScrollOffset);
        int    radiusRounded = int(radius + 0.5f);
        
        positionScr += screenPos;
        
        IRect boundingBox(positionScr.x - radiusRounded - 2, positionScr.y - radiusRounded - 2, positionScr.x + radiusRounded + 2, positionScr.y + radiusRounded + 2);
        for (const IRect& clip : region->m_Rects)
        {
            IRect clipRect = clip + screenPos;
            if (!boundingBox.DoIntersect(clipRect)) {
                continue;
            }
            m_Bitmap->m_Driver->FillCircle(m_Bitmap, clipRect, positionScr, int32_t(roundf(radius)), m_FgColor, m_DrawingMode);
        }
    }    
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerView::DrawString(const String& string)
{
    Ptr<const Region> region = GetRegion();
    if (region != nullptr)
    {
        IPoint screenPos(m_ScreenPos);
        IPoint penPos = IPoint(m_PenPosition + m_ScrollOffset);
        
        DisplayDriver* driver = m_Bitmap->m_Driver;

        IRect boundingBox(penPos.x, penPos.y, penPos.x + int(driver->GetStringWidth(m_Font->Get(), string.c_str(), string.size())), penPos.y + int(driver->GetFontHeight(m_Font->Get())));

        penPos += screenPos;
        
        for (const IRect& clip : region->m_Rects)
        {
            if (clip.DoIntersect(boundingBox))
            {
                driver->WriteString(m_Bitmap, penPos, string.c_str(), string.size(), clip + screenPos, m_BgColor, m_FgColor, m_Font->Get());
            }                
        }
        m_PenPosition.x += float(boundingBox.Width());
    }        
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerView::CopyRect(const Rect& srcRect, const Point& dstPos)
{
    if ( m_VisibleReg == nullptr ) {
        return;
    }

    IRect  intSrcRect(srcRect);
    IPoint delta   = IPoint(dstPos) - intSrcRect.TopLeft();

    if (delta.x == 0 && delta.y == 0) {
        return;
    }

    intSrcRect += IPoint(m_ScrollOffset);
    IRect  dstRect = intSrcRect + delta;

    std::vector<IRect> bltList;
    Region       damage(*m_VisibleReg, intSrcRect, false);

    for (const IRect& srcClip : m_VisibleReg->m_Rects)
    {
        // Clip to source rectangle
        IRect sRect(intSrcRect & srcClip);

        if (!sRect.IsValid()) {
            continue;
        }
        // Transform into destination space
        sRect += delta;

        for(const IRect& dstClip : m_VisibleReg->m_Rects)
        {
            IRect dRect = sRect & dstClip;

            if (!dRect.IsValid()) {
                continue;
            }
            damage.Exclude(dRect);
            bltList.push_back(dRect);
        }
    }

    
    if (bltList.empty())
    {
        Invalidate(dstRect);
        UpdateIfNeeded(true);
        return;
    }

    std::vector<IRect*> clipList;
    clipList.reserve(bltList.size());
    
    for (IRect& clip : bltList) {
        clipList.push_back(&clip);
    }
    std::sort(clipList.begin(), clipList.end(), BlitSortCompare(delta));

    IPoint screenPos(m_ScreenPos);
    for (IRect* clip : clipList)
    {
        *clip += screenPos; // Convert into screen space
        m_Bitmap->m_Driver->CopyRect(m_Bitmap, m_Bitmap, m_BgColor, m_FgColor, *clip - delta, clip->TopLeft(), m_DrawingMode);
    }
    if (m_DamageReg != nullptr)
    {
        Region region(*m_DamageReg, intSrcRect, false);
        for (const IRect& dmgClip : region.m_Rects)
        {
            m_DamageReg->Include((dmgClip + delta)  & dstRect);
            if (m_ActiveDamageReg != nullptr) {
                m_ActiveDamageReg->Exclude((dmgClip + delta)  & dstRect);
            }
        }
    }
    if (m_ActiveDamageReg != nullptr)
    {
        Region region(*m_ActiveDamageReg, intSrcRect, false);
        if (!region.IsEmpty())
        {
            if ( m_DamageReg == nullptr ) {
                m_DamageReg = ptr_new<Region>();
            }
            for (const IRect& dmgClip : region.m_Rects)
            {
                m_ActiveDamageReg->Exclude((dmgClip + delta) & dstRect);
                m_DamageReg->Include((dmgClip + delta) & dstRect);
            }
        }
    }
    for (const IRect& dstClip : damage.m_Rects) {
        Invalidate(dstClip);
    }
    if ( m_DamageReg != nullptr ) {
        m_DamageReg->Optimize();
    }
    if (m_ActiveDamageReg != nullptr) {
        m_ActiveDamageReg->Optimize();
    }
    UpdateIfNeeded(true);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerView::DrawBitmap(Ptr<SrvBitmap> bitmap, const Rect& srcRect, const Point& dstPos)
{
    if (bitmap == nullptr || m_VisibleReg == nullptr) {
        return;
    }
    Ptr<const Region> region = GetRegion();

    if (region == nullptr) {
        return;
    }

    IPoint screenPos(m_ScreenPos);

    IRect  intSrcRect(srcRect);
    IPoint intDstPos(dstPos + m_ScrollOffset);

    IRect clippedSrcRect = intSrcRect & IRect(IPoint(0, 0), bitmap->m_Size);
    intDstPos += clippedSrcRect.TopLeft() - intSrcRect.TopLeft();

    IRect  dstRect = clippedSrcRect.Bounds() + intDstPos;
    IPoint srcPos(clippedSrcRect.TopLeft());

    for (const IRect& clipRect : region->m_Rects)
    {
        IRect rect = dstRect & clipRect;

        if (rect.IsValid())
        {
            IPoint cDst = rect.TopLeft() + screenPos;
            IRect  cSrc = rect - intDstPos + srcPos;

            m_Bitmap->m_Driver->CopyRect(m_Bitmap, ptr_raw_pointer_cast(bitmap), m_BgColor, m_FgColor, cSrc, cDst, m_DrawingMode);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerView::DebugDraw(Color color, uint32_t drawFlags)
{
    IPoint screenPos(m_ScreenPos);
    
    if (drawFlags & ViewDebugDrawFlags::ViewFrame)
    {
        DebugDrawRect(GetNormalizedIBounds() + screenPos, color);
    }
    if (drawFlags & ViewDebugDrawFlags::DrawRegion)
    {
        if (m_VisibleReg != nullptr)
        {
            for (const IRect& clip : m_VisibleReg->m_Rects)
            {
                DebugDrawRect(clip + screenPos, color);
            }
        }
    }        
    if (drawFlags & ViewDebugDrawFlags::DamageRegion)
    {
        Ptr<Region> region = GetRegion();
        if (region != nullptr)
        {
            for (const IRect& clip : region->m_Rects)
            {
                DebugDrawRect(clip + screenPos, color);
            }
        }
    }        
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerView::ScrollBy(const Point& offset)
{
    Ptr<ServerView> parent = m_Parent.Lock();
    if ( parent == nullptr ) {
        return;
    }
    IPoint screenPos(m_ScreenPos);

    IPoint oldOffset(m_ScrollOffset);
    m_ScrollOffset += offset;
    IPoint newOffset(m_ScrollOffset);
    
    if (newOffset == oldOffset) {
        return;
    }
    UpdateScreenPos();
    IPoint intOffset(newOffset - oldOffset);
    
    if ( m_HideCount > 0 ) {
        return;
    }

    UpdateRegions();
    //SrvWindow::HandleMouseTransaction();
    
    if (m_FullReg == nullptr /*|| m_pcBitmap == nullptr*/ ) {
        return;
    }

    IRect        bounds(GetNormalizedBounds());
    std::vector<IRect> bltList;
    Region       damage(*m_VisibleReg);

    for (const IRect& srcClip : m_FullReg->m_Rects)
    {
        // Clip to source rectangle
        IRect sRect = bounds & srcClip;

        // Transform into destination space
        if (!sRect.IsValid()) {
            continue;
        }
        sRect += intOffset;

        for(const IRect& dstClip : m_FullReg->m_Rects)
        {
            IRect dRect = sRect & dstClip;
            if (dRect.IsValid()) {
                damage.Exclude(dRect);
                bltList.push_back(dRect);
            }                
        }
    }
  
    if (bltList.empty())
    {
        Invalidate(bounds);
        UpdateIfNeeded(true);
        return;
    }

    std::vector<IRect*> clipList;
    clipList.reserve(bltList.size());

    for (IRect& clip : bltList) {
        clipList.push_back(&clip);
    }
    std::sort(clipList.begin(), clipList.end(), BlitSortCompare(intOffset));

    for (size_t i = 0 ; i < clipList.size() ; ++i)
    {
        IRect* clip = clipList[i];

        *clip += screenPos; // Convert into screen space

        m_Bitmap->m_Driver->CopyRect(m_Bitmap, m_Bitmap, m_BgColor, m_FgColor, *clip - intOffset, clip->TopLeft(), DrawingMode::Copy);
    }

    if (m_DamageReg != nullptr)
    {
        for (IRect& dstClip : m_DamageReg->m_Rects) {
            dstClip += intOffset;
        }
    }

    if (m_ActiveDamageReg != nullptr)
    {
        for (IRect& dstClip : m_ActiveDamageReg->m_Rects) {
            dstClip += intOffset;
        }
    }
    for (const IRect& dstClip : damage.m_Rects) {
        Invalidate(dstClip);
    }
    UpdateIfNeeded(true);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerView::UpdateScreenPos()
{
    Ptr<ServerView> parent = m_Parent.Lock();
    if (parent == nullptr) {
        m_ScreenPos = m_Frame.TopLeft();
    }
    else {
        m_ScreenPos = parent->m_ScreenPos + parent->m_ScrollOffset + m_Frame.TopLeft();
    }
    for (Ptr<ServerView> child : m_ChildrenList) {
        child->UpdateScreenPos();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ServerView::DebugDrawRect(const IRect& frame, Color color)
{
    IPoint p1(frame.left, frame.top);
    IPoint p2(frame.right - 1, frame.top);
    IPoint p3(frame.right - 1, frame.bottom - 1);
    IPoint p4(frame.left, frame.bottom - 1);

    m_Bitmap->m_Driver->DrawLine(m_Bitmap, frame, p1, p2, color, DrawingMode::Copy);
    m_Bitmap->m_Driver->DrawLine(m_Bitmap, frame, p2, p3, color, DrawingMode::Copy);
    m_Bitmap->m_Driver->DrawLine(m_Bitmap, frame, p4, p3, color, DrawingMode::Copy);
    m_Bitmap->m_Driver->DrawLine(m_Bitmap, frame, p1, p4, color, DrawingMode::Copy);
}
