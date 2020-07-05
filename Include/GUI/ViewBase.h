// This file is part of PadOS.
//
// Copyright (C) 2020 Kurt Skauen <http://kavionic.com/>
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
// Created: 29.06.2020 22:35

#pragma once

#include "GUIDefines.h"

namespace os
{

template<typename ViewType>
class ViewBase : public EventHandler, public SignalTarget
{
public:
    typedef std::vector<Ptr<ViewType>> ChildList_t;

    ViewBase(const String& name, const Rect& frame, const Point& scrollOffset, uint32_t flags, int32_t hideCount, Color eraseColor, Color bgColor, Color fgColor)
        : EventHandler(name)
        , m_Frame(frame)
        , m_ScrollOffset(scrollOffset)
        , m_Flags(flags)
        , m_HideCount(hideCount)
        , m_EraseColor(eraseColor)
        , m_BgColor(bgColor)
        , m_FgColor(fgColor) {}

    Ptr<ViewType>       GetParent()       { return m_Parent.Lock(); }
    Ptr<const ViewType> GetParent() const { return m_Parent.Lock(); }

    const ChildList_t& GetChildList() const { return m_ChildrenList; }
        
    typename ChildList_t::iterator                  begin() { return m_ChildrenList.begin(); }
    typename ChildList_t::iterator                  end()   { return m_ChildrenList.end(); }
        
    typename ChildList_t::const_iterator            begin() const { return m_ChildrenList.begin(); }
    typename ChildList_t::const_iterator            end() const   { return m_ChildrenList.end(); }

    typename ChildList_t::reverse_iterator          rbegin() { return m_ChildrenList.rbegin(); }
    typename ChildList_t::reverse_iterator          rend()   { return m_ChildrenList.rend(); }

    typename ChildList_t::const_reverse_iterator    rbegin() const { return m_ChildrenList.rbegin(); }
    typename ChildList_t::const_reverse_iterator    rend() const   { return m_ChildrenList.rend(); }
        
    typename ChildList_t::iterator                  GetChildIterator(Ptr<ViewType> child)       { return std::find(m_ChildrenList.begin(), m_ChildrenList.end(), child); }
    typename ChildList_t::const_iterator            GetChildIterator(Ptr<ViewType> child) const { return std::find(m_ChildrenList.begin(), m_ChildrenList.end(), child); }

    typename ChildList_t::reverse_iterator          GetChildRIterator(Ptr<ViewType> child)       { return std::find(m_ChildrenList.rbegin(), m_ChildrenList.rend(), child); }
    typename ChildList_t::const_reverse_iterator    GetChildRIterator(Ptr<ViewType> child) const { return std::find(m_ChildrenList.rbegin(), m_ChildrenList.rend(), child); }

    template<typename T>
    Ptr<T> FindChild(const String& name, bool recursive = true)
    {
	    return ptr_dynamic_cast<T>(FindChildInternal(name, recursive));
    }

    
    Ptr<ViewType> FindChildInternal(const String& name, bool recursive = true)
    {
        for (const Ptr<ViewType>& child : m_ChildrenList)
        {
            if (child->GetName() == name) return child;
        }
        if (recursive)
        {
            for (const Ptr<ViewType>& child : m_ChildrenList)
            {
                Ptr<ViewType> view = child->FindChildInternal(name, true);
                if (view != nullptr) return view;
            }
        }
        return nullptr;
    }

    int32_t GetChildIndex(Ptr<ViewType> child) const
    {
        auto i = GetChildIterator(child);
        if (i != m_ChildrenList.end()) {
            return std::distance(m_ChildrenList.begin(), i);
        } else {
            return -1;
        }
    }
    virtual void OnFlagsChanged(uint32_t oldFlags) {}

    void        ReplaceFlags(uint32_t flags)	    { uint32_t oldFlags = m_Flags; if (flags != m_Flags) { m_Flags = flags; OnFlagsChanged(oldFlags); } }
    void        MergeFlags(uint32_t flags)	        { ReplaceFlags(m_Flags | flags); }
    void        ClearFlags(uint32_t flags)	        { ReplaceFlags(m_Flags & ~flags); }
    uint32_t    GetFlags() const		            { return m_Flags; }
    bool	    HasFlags(uint32_t flags) const	    { return (m_Flags & flags) != 0; }
    bool	    HasFlagsAll(uint32_t mask) const    { return (m_Flags & mask) == mask; }
    

    const Rect&	GetFrame() const { return m_Frame; }
    Rect	GetBounds() const { return m_Frame.Bounds() /*- Point(m_Frame.left, m_Frame.top)*/ - m_ScrollOffset; }
    Rect	GetNormalizedBounds() const { return m_Frame.Bounds(); }

    IRect	GetIFrame() const { return IRect(m_Frame); }
    IRect	GetIBounds() const { return GetIFrame().Bounds() /*- Point(m_Frame.left, m_Frame.top)*/ - IPoint(m_ScrollOffset); }
    IRect	GetNormalizedIBounds() const { return GetIFrame().Bounds(); }
        
    Point	GetTopLeft() const  { return Point( m_Frame.left, m_Frame.top ); }
    IPoint	GetITopLeft() const { return IPoint(m_Frame.TopLeft()); }
        
    Color	GetFgColor() const { return m_FgColor; }
    Color	GetBgColor() const { return m_BgColor; }
    Color	GetEraseColor() const { return m_EraseColor; }

    
      // Coordinate conversions:
    Point       ConvertToParent(const Point& point) const   { return point + GetTopLeft(); }
    void        ConvertToParent(Point* point) const         { *point += GetTopLeft(); }
    Rect        ConvertToParent(const Rect& rect) const     { return rect + GetTopLeft(); }
    void        ConvertToParent(Rect* rect) const           { *rect += GetTopLeft(); }
    Point       ConvertFromParent(const Point& point) const { return point - GetTopLeft(); }
    void        ConvertFromParent(Point* point) const       { *point -= GetTopLeft(); }
    Rect        ConvertFromParent(const Rect& rect) const   { return rect - GetTopLeft(); }
    void        ConvertFromParent(Rect* rect) const         { *rect -= GetTopLeft(); }
    Point       ConvertToRoot(const Point& point) const     { return m_ScreenPos + point; }
    void        ConvertToRoot(Point* point) const           { *point += m_ScreenPos; }
    Rect        ConvertToRoot(const Rect& rect) const       { return rect + m_ScreenPos; }
    void        ConvertToRoot(Rect* rect) const             { *rect += m_ScreenPos; }
    Point       ConvertFromRoot(const Point& point) const   { return point - m_ScreenPos; }
    void        ConvertFromRoot(Point* point) const         { *point -= m_ScreenPos; }
    Rect        ConvertFromRoot(const Rect& rect) const     { return rect - m_ScreenPos; }
    void        ConvertFromRoot(Rect* rect) const           { *rect -= m_ScreenPos; }
    
    static Ptr<ViewType> GetOpacParent(Ptr<ViewType> view, IRect* frame)
    {
        while(view != nullptr && view->HasFlags(ViewFlags::Transparent))
        {
            if (frame != nullptr) {
                *frame += view->GetITopLeft();
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
            m_ScreenPos = m_Frame.TopLeft();
        } else {
            m_ScreenPos = parent->m_ScreenPos + m_Frame.TopLeft();
        }
        for (Ptr<ViewBase> child : m_ChildrenList) {
            child->Added(this, hideCount, level + 1);
        }
    }

    void UpdateScreenPos()
    {
        Ptr<ViewType> parent = m_Parent.Lock();
        if (parent == nullptr) {
            m_ScreenPos = m_Frame.TopLeft();
        } else {
            m_ScreenPos = parent->m_ScreenPos + parent->m_ScrollOffset + m_Frame.TopLeft();
        }
        for (Ptr<ViewType> child : m_ChildrenList) {
            child->UpdateScreenPos();
        }
    }

    Rect        m_Frame = Rect(0.0f, 0.0f, 0.0f, 0.0f);
    Point       m_ScrollOffset;
    uint32_t    m_Flags = 0;    

    Point       m_ScreenPos = Point(0.0f, 0.0f);

    WeakPtr<ViewType>   m_Parent;
    ChildList_t         m_ChildrenList;
    
    Point       m_PenPosition;

    int         m_HideCount = 0;
    int         m_Level = 0;

    Color       m_EraseColor = Color(0xffffffff);
    Color       m_BgColor    = Color(0xffffffff);
    Color       m_FgColor    = Color(0xff000000);
    
    ViewBase(const ViewBase&) = delete;
    ViewBase& operator=(const ViewBase&) = delete;
};


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template<typename ViewType>
void ViewBase<ViewType>::LinkChild(Ptr<ViewType> child, bool topmost)
{
	if (child->m_Parent.Lock() == nullptr)
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
		printf("ERROR : Attempt to add a view already belonging to a window\n");
	}
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template<typename ViewType>
void ViewBase<ViewType>::UnlinkChild(Ptr<ViewType> child)
{
	if (child->m_Parent.Lock() == static_cast<ViewType*>(this))
	{
		child->m_Parent = nullptr;

		auto i = GetChildIterator(child);
		if (i != m_ChildrenList.end()) {
			m_ChildrenList.erase(i);
		} else {
			printf("ERROR: ViewBase::UnlinkChildren() failed to find view in children list.\n");
		}
		child->UpdateScreenPos();
		child->HandleRemovedFromParent(ptr_tmp_cast(static_cast<ViewType*>(this)));
	}
    else
	{
		printf("ERROR : Attempt to remove a view not belonging to this window\n");
	}
}

} // namespace