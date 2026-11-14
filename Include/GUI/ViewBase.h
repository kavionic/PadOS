// This file is part of PadOS.
//
// Copyright (c) 2020-2021 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 29.06.2020 22:35

#pragma once

#include <concepts>
#include <vector>

#include "GUIDefines.h"

template<typename ViewType>
class PViewBase : public PEventHandler, public SignalTarget
{
public:
    typedef std::vector<Ptr<ViewType>> ChildList_t;

    PViewBase(
        const PString& name,
        const PRect& frame,
        const PPoint& scrollOffset,
        PViewFlags flags,
        PViewHitMode hitMode,
        int32_t hideCount,
        float penWidth,
        PColor fgColor,
        PCapStyle capStyle = PCapStyle::Flat,
        PJointStyle jointStyle = PJointStyle::Bevel,
        float miterLimit = 4.0f,
        const std::vector<float>& dashPattern = {},
        float dashOffset = 0.0f)
        : PEventHandler(name)
        , m_Frame(frame)
        , m_ScrollOffset(scrollOffset)
        , m_Flags(flags)
        , m_HitMode(hitMode)
        , m_CapStyle(capStyle)
        , m_JointStyle(jointStyle)
        , m_PenWidth(penWidth)
        , m_MiterLimit(miterLimit)
        , m_DashPattern(dashPattern)
        , m_DashOffset(dashOffset)
        , m_HideCount(hideCount)
        , m_FgColor(fgColor) {}

    Ptr<ViewType>       GetParent()       { return m_Parent.Lock(); }
    Ptr<const ViewType> GetParent() const { return ptr_static_cast<const ViewType>(m_Parent.Lock()); }

    Ptr<ViewType>       GetRoot()       { Ptr<ViewType> parent = GetParent(); return (parent != nullptr) ? parent->GetRoot() : ptr_static_cast<ViewType>(ptr_tmp_cast(this)); }
    Ptr<const ViewType> GetRoot() const { Ptr<const ViewType> parent = GetParent(); return (parent != nullptr) ? parent->GetRoot() : ptr_static_cast<const ViewType>(ptr_tmp_cast(this)); }

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

    template<typename T = PView, typename DELEGATE>
    Ptr<T> FindChildIf(DELEGATE compareDelegate, bool recursive = true) const
    {
        return ptr_dynamic_cast<T>(FindChildInternal(compareDelegate, recursive));
    }

    template<typename T = PView, typename DELEGATE>
    bool FindChildIf(DELEGATE compareDelegate, Ptr<T>& outResult, bool recursive = true) const
    {
        outResult = FindChildIf<T>(compareDelegate, recursive);
        return outResult != nullptr;
    }

    template<typename T = PView>
    Ptr<T> FindChild(const PString& name, bool recursive = true) const
    {
        return ptr_dynamic_cast<T>(FindChildInternal([&name](Ptr<ViewType> child) { return child->GetName() == name; }, recursive));
    }

    template<typename T = PView>
    bool FindChild(const PString& name, Ptr<T>& outResult, bool recursive = true) const
    {
        outResult = FindChild<T>(name, recursive);
        return outResult != nullptr;
    }

    template<typename DELEGATE>
    Ptr<ViewType> FindChildInternal(DELEGATE compareDelegate, bool recursive = true) const
    {
        for (const Ptr<ViewType>& child : m_ChildrenList)
        {
            if (compareDelegate(child)) return child;
        }
        if (recursive)
        {
            for (const Ptr<ViewType>& child : m_ChildrenList)
            {
                Ptr<ViewType> view = child->FindChildInternal(compareDelegate, true);
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
    virtual void OnFlagsChanged(PViewFlags changedFlags) {}

    template<PRegisteredBitmaskEnum TFlag>
        requires std::derived_from<PEnumBitmaskFor<TFlag>, PViewFlags>
    void ReplaceFlags(TFlag flag)
    {
        ReplaceFlags<PEnumBitmaskFor<TFlag>>(flag);
    }

    template<PEnumBitmaskType TFlags = PViewFlags>
        requires std::derived_from<TFlags, PViewFlags>
    void ReplaceFlags(std::type_identity_t<TFlags> flags)
    {
        if (flags != m_Flags)
        {
            const PViewFlags changedFlags = m_Flags ^ PViewFlags(flags);
            m_Flags = flags;
            OnFlagsChanged(changedFlags);
        }
    }

    template<PRegisteredBitmaskEnum TFlag>
        requires std::derived_from<PEnumBitmaskFor<TFlag>, PViewFlags>
    void MergeFlags(TFlag flag)
    {
        MergeFlags<PEnumBitmaskFor<TFlag>>(flag);
    }

    template<PEnumBitmaskType TFlags = PViewFlags>
        requires std::derived_from<TFlags, PViewFlags>
    void MergeFlags(std::type_identity_t<TFlags> flags)
    {
        ReplaceFlags(m_Flags | PViewFlags(flags));
    }

    template<PRegisteredBitmaskEnum TFlag>
        requires std::derived_from<PEnumBitmaskFor<TFlag>, PViewFlags>
    void ClearFlags(TFlag flag)
    {
        ClearFlags<PEnumBitmaskFor<TFlag>>(flag);
    }

    template<PEnumBitmaskType TFlags = PViewFlags>
        requires std::derived_from<TFlags, PViewFlags>
    void ClearFlags(std::type_identity_t<TFlags> flags)
    {
        ReplaceFlags(m_Flags & ~PViewFlags(flags));
    }

    template<PEnumBitmaskType TFlags = PViewFlags>
        requires std::derived_from<TFlags, PViewFlags>
    TFlags GetFlags() const
    {
        return TFlags(m_Flags);
    }

    template<PRegisteredBitmaskEnum TFlag>
        requires std::derived_from<PEnumBitmaskFor<TFlag>, PViewFlags>
    bool HasFlags(TFlag flag) const
    {
        return HasFlags<PEnumBitmaskFor<TFlag>>(flag);
    }

    template<PEnumBitmaskType TFlags = PViewFlags>
        requires std::derived_from<TFlags, PViewFlags>
    bool HasFlags(std::type_identity_t<TFlags> flags) const
    {
        return GetFlags<TFlags>().HasAny(flags);
    }

    template<PRegisteredBitmaskEnum TFlag>
        requires std::derived_from<PEnumBitmaskFor<TFlag>, PViewFlags>
    bool HasFlagsAll(TFlag flag) const
    {
        return HasFlagsAll<PEnumBitmaskFor<TFlag>>(flag);
    }

    template<PEnumBitmaskType TFlags = PViewFlags>
        requires std::derived_from<TFlags, PViewFlags>
    bool HasFlagsAll(std::type_identity_t<TFlags> flags) const
    {
        return GetFlags<TFlags>().HasAll(flags);
    }

    const PRect&	GetFrame() const { return m_Frame; }
    PRect	    GetBounds() const { return m_Frame.Bounds() - m_ScrollOffset; }
    PRect	    GetNormalizedBounds() const { return m_Frame.Bounds(); }

    PIRect	    GetIFrame() const { return PIRect(m_Frame); }
    PIRect	    GetIBounds() const { return GetIFrame().Bounds() - PIPoint(m_ScrollOffset); }
    PIRect	    GetNormalizedIBounds() const { return GetIFrame().Bounds(); }
        
    PPoint	    GetTopLeft() const  { return PPoint( m_Frame.left, m_Frame.top ); }
    PIPoint	    GetITopLeft() const { return PIPoint(m_Frame.TopLeft()); }
        
    PPoint      GetScrollOffset() const { return m_ScrollOffset; }

    PColor	    GetFgColor() const { return m_FgColor; }

    
      // Coordinate conversions:
    PPoint      ConvertToParent(const PPoint& point) const   { return point + GetTopLeft() + m_ScrollOffset; }
    void        ConvertToParent(PPoint* point) const         { *point += GetTopLeft() + m_ScrollOffset; }
    PRect       ConvertToParent(const PRect& rect) const     { return rect + GetTopLeft() + m_ScrollOffset; }
    void        ConvertToParent(PRect* rect) const           { *rect += GetTopLeft() + m_ScrollOffset; }
    PPoint      ConvertFromParent(const PPoint& point) const { return point - GetTopLeft() - m_ScrollOffset; }
    void        ConvertFromParent(PPoint* point) const       { *point -= GetTopLeft() + m_ScrollOffset; }
    PRect       ConvertFromParent(const PRect& rect) const   { return rect - GetTopLeft() - m_ScrollOffset; }
    void        ConvertFromParent(PRect* rect) const         { *rect -= GetTopLeft() + m_ScrollOffset; }
    
    PPoint      ConvertToRoot(const PPoint& point) const     { return m_ScreenPos + point + m_ScrollOffset - GetRootScreenPosition(); }
    void        ConvertToRoot(PPoint* point) const           { *point += m_ScreenPos + m_ScrollOffset - GetRootScreenPosition(); }
    PRect       ConvertToRoot(const PRect& rect) const       { return rect + m_ScreenPos + m_ScrollOffset - GetRootScreenPosition(); }
    void        ConvertToRoot(PRect* rect) const             { *rect += m_ScreenPos + m_ScrollOffset - GetRootScreenPosition(); }
    PPoint      ConvertFromRoot(const PPoint& point) const   { return point - m_ScreenPos - m_ScrollOffset + GetRootScreenPosition(); }
    void        ConvertFromRoot(PPoint* point) const         { *point -= m_ScreenPos + m_ScrollOffset - GetRootScreenPosition(); }
    PRect       ConvertFromRoot(const PRect& rect) const     { return rect - m_ScreenPos - m_ScrollOffset + GetRootScreenPosition(); }
    void        ConvertFromRoot(PRect* rect) const           { *rect -= m_ScreenPos + m_ScrollOffset - GetRootScreenPosition(); }

    PPoint      ConvertToScreen(const PPoint& point) const   { return m_ScreenPos + point + m_ScrollOffset; }
    void        ConvertToScreen(PPoint* point) const         { *point += m_ScreenPos + m_ScrollOffset; }
    PRect       ConvertToScreen(const PRect& rect) const     { return rect + m_ScreenPos + m_ScrollOffset; }
    void        ConvertToScreen(PRect* rect) const           { *rect += m_ScreenPos + m_ScrollOffset; }
    PPoint      ConvertFromScreen(const PPoint& point) const { return point - m_ScreenPos - m_ScrollOffset; }
    void        ConvertFromScreen(PPoint* point) const       { *point -= m_ScreenPos + m_ScrollOffset; }
    PRect       ConvertFromScreen(const PRect& rect) const   { return rect - m_ScreenPos - m_ScrollOffset; }
    void        ConvertFromScreen(PRect* rect) const         { *rect -= m_ScreenPos + m_ScrollOffset; }
    
    static Ptr<ViewType> GetOpacParent(Ptr<ViewType> view, PIRect* frame = nullptr)
    {
        while(view != nullptr && view->HasFlags(PViewFlag::Transparent))
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
    friend class PApplication;
    friend class ApplicationServer;
    friend class ServerApplication;

    PPoint GetRootScreenPosition() const
    {
        const Ptr<const ViewType> root = GetRoot();
        return root->m_ScreenPos;
    }

    void            LinkChild(Ptr<ViewType> child, size_t index);
    Ptr<ViewType>   UnlinkChild(typename  ChildList_t::iterator iterator);
    void            UnlinkChild(Ptr<ViewType> child);
    
    void Added(PViewBase* parent, int level)
    {
        m_Level = level;
        if (parent == nullptr) {
            m_ScreenPos = m_Frame.TopLeft();
        } else {
            m_ScreenPos = parent->m_ScreenPos + parent->m_ScrollOffset + m_Frame.TopLeft();
        }
        for (Ptr<PViewBase> child : m_ChildrenList) {
            child->Added(this, level + 1);
        }
    }

    PRect           m_Frame = PRect(0.0f, 0.0f, 0.0f, 0.0f);
    PPoint          m_ScrollOffset;
    PViewFlags      m_Flags = PViewFlags::zero;    
    PViewHitMode    m_HitMode = PViewHitMode::HitTest;
    PCapStyle       m_CapStyle = PCapStyle::Flat;
    PJointStyle     m_JointStyle = PJointStyle::Bevel;

    PPoint          m_ScreenPos = PPoint(0.0f, 0.0f);

    WeakPtr<ViewType>   m_Parent;
    std::vector<Ptr<ViewType>>  m_ChildrenList;
    
    PPoint      m_PenPosition;
    float       m_PenWidth   = 1.0f;

    float               m_MiterLimit = 4.0f;
    std::vector<float>  m_DashPattern;
    float               m_DashOffset = 0.0f;

    int         m_HideCount = 0;
    int         m_Level = 0;

    PColor       m_FgColor      = PColor::FromRGB32A(0, 0, 0);
    
    PViewBase(const PViewBase&) = delete;
    PViewBase& operator=(const PViewBase&) = delete;
};


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template<typename ViewType>
void PViewBase<ViewType>::LinkChild(Ptr<ViewType> child, size_t index)
{
	if (child->m_Parent.Lock() == nullptr)
	{
		child->m_Parent = ptr_tmp_cast(static_cast<ViewType*>(this));
		if (index == INVALID_INDEX) {
			m_ChildrenList.push_back(child);
		} else {
			m_ChildrenList.insert(m_ChildrenList.begin() + index, child);
		}
		child->Added(this, m_Level + 1);
		child->HandleAddedToParent(ptr_tmp_cast(static_cast<ViewType*>(this)), index);
	}
    else
	{
        p_system_log<PLogSeverity::ERROR>(LogCategoryGUITK, "Attempt to add a view already belonging to a window.");
	}
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template<typename ViewType>
Ptr<ViewType> PViewBase<ViewType>::UnlinkChild(typename  ChildList_t::iterator iterator)
{
    Ptr<ViewType> child = *iterator;
    if (child->m_Parent.Lock() == static_cast<ViewType*>(this))
    {
        child->m_Parent = nullptr;

        m_ChildrenList.erase(iterator);

        child->HandleRemovedFromParent(ptr_tmp_cast(static_cast<ViewType*>(this)));
    }
    else
    {
        p_system_log<PLogSeverity::ERROR>(LogCategoryGUITK, "Attempt to remove a view not belonging to this window.");
    }
    return child;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template<typename ViewType>
void PViewBase<ViewType>::UnlinkChild(Ptr<ViewType> child)
{
	if (child->m_Parent.Lock() == static_cast<ViewType*>(this))
	{
		auto i = GetChildIterator(child);
		if (i != m_ChildrenList.end()) {
            UnlinkChild(i);
		} else {
            p_system_log<PLogSeverity::ERROR>(LogCategoryGUITK, "ViewBase::UnlinkChildren() failed to find view in children list.");
		}
	}
    else
	{
        p_system_log<PLogSeverity::ERROR>(LogCategoryGUITK, "Attempt to remove a view not belonging to this window.");
	}
}
