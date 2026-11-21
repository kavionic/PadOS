// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <GUI/Style/GUIStyle.h>
#include <System/ExceptionHandling.h>
#include <GUI/Style/ViewStyleButton.h>
#include <GUI/Style/ViewStyleDropdownMenu.h>
#include <GUI/Style/ViewStyleDropdownMenuPopup.h>
#include <GUI/Style/ViewStyleDropdownMenuPopupFrame.h>
#include <GUI/Style/ViewStyleGroupView.h>
#include <GUI/Style/ViewStyleProgressBar.h>
#include <GUI/Style/ViewStyleScrollBar.h>
#include <GUI/Style/ViewStyleSlider.h>
#include <GUI/Style/ViewStyleTabView.h>

class PViewRendererTestView :
    public PView,
    public PViewStyleButton,
    public PViewStyleSlider,
    public PViewStyleScrollBar,
    public PViewStyleDropdownMenu,
    public PViewStyleDropdownMenuPopupFrame,
    public PViewStyleDropdownMenuPopup,
    public PViewStyleGroupView,
    public PViewStyleProgressBar,
    public PViewStyleTabView
{
public:
    explicit PViewRendererTestView(const PString& name, PViewFlags flags = PViewFlags::zero)
        : PView(name, flags)
        , PViewStyleButton(this)
        , PViewStyleSlider(this)
        , PViewStyleScrollBar(this)
        , PViewStyleDropdownMenu(this)
        , PViewStyleDropdownMenuPopupFrame(this)
        , PViewStyleDropdownMenuPopup(this)
        , PViewStyleGroupView(this)
        , PViewStyleProgressBar(this)
        , PViewStyleTabView(this)
    {
    }

    virtual const PButtonStyleData& GetButtonStyleData() const override { return m_ButtonData; }
    PButtonStyleData& GetButtonStyleData() { return m_ButtonData; }

    virtual const PSliderStyleData& GetSliderStyleData() const override { return m_SliderData; }
    PSliderStyleData& GetSliderStyleData() { return m_SliderData; }

    virtual const PScrollBarStyleData& GetScrollBarStyleData() const override { return m_ScrollBarData; }
    PScrollBarStyleData& GetScrollBarStyleData() { return m_ScrollBarData; }

    virtual const PDropdownMenuStyleData& GetDropdownMenuStyleData() const override { return m_DropdownMenuData; }
    PDropdownMenuStyleData& GetDropdownMenuStyleData() { return m_DropdownMenuData; }

    virtual const PDropdownMenuPopupStyleData& GetDropdownMenuPopupStyleData() const override { return m_DropdownMenuPopupData; }
    PDropdownMenuPopupStyleData& GetDropdownMenuPopupStyleData() { return m_DropdownMenuPopupData; }

    virtual const PGroupViewStyleData& GetGroupViewStyleData() const override { return m_GroupViewData; }
    PGroupViewStyleData& GetGroupViewStyleData() { return m_GroupViewData; }

    virtual const PProgressBarStyleData& GetProgressBarStyleData() const override { return m_ProgressBarData; }
    PProgressBarStyleData& GetProgressBarStyleData() { return m_ProgressBarData; }

    virtual const PTabViewStyleData& GetTabViewStyleData() const override { return m_TabViewData; }
    PTabViewStyleData& GetTabViewStyleData() { return m_TabViewData; }

    virtual bool IsButtonEnabled() const override { return m_IsButtonEnabled; }
    void SetButtonEnabled(bool enabled) { m_IsButtonEnabled = enabled; }

    virtual PPoint GetPopupContentSize() const override { return m_PopupContentSize; }
    void SetPopupContentSize(const PPoint& size) { m_PopupContentSize = size; }

    std::vector<PString>& GetPopupItems() { return m_PopupItems; }
    const std::vector<PString>& GetPopupItems() const { return m_PopupItems; }
    void SetPopupItems(const std::vector<PString>& items) { m_PopupItems = items; }

    virtual PView& GetTabsView() const override
    {
        assert(m_TabsView != nullptr);
        return *m_TabsView;
    }
    void SetTabsView(PView& view) { m_TabsView = &view; }

    virtual void GetTabContentSizeLimits(PPoint* minSize, PPoint* maxSize) const override
    {
        *minSize = m_ContentMinSize;
        *maxSize = m_ContentMaxSize;
    }
    void SetTabContentSizeLimits(const PPoint& minSize, const PPoint& maxSize)
    {
        m_ContentMinSize = minSize;
        m_ContentMaxSize = maxSize;
    }

private:
    std::vector<PString> m_PopupItems;
    PButtonStyleData m_ButtonData;
    PSliderStyleData m_SliderData;
    PScrollBarStyleData m_ScrollBarData;
    PDropdownMenuStyleData m_DropdownMenuData;
    PDropdownMenuPopupStyleData m_DropdownMenuPopupData{m_PopupItems};
    PGroupViewStyleData m_GroupViewData;
    PProgressBarStyleData m_ProgressBarData;
    PTabViewStyleData m_TabViewData;
    PPoint m_PopupContentSize;
    PPoint m_ContentMinSize;
    PPoint m_ContentMaxSize;
    PView* m_TabsView = nullptr;
    bool m_IsButtonEnabled = true;
};

// A test binding makes setup/teardown and per-view context ownership explicit.
// Declare it after its target so teardown runs while the interface is alive.
template<class TRendererType>
class PViewRendererTestBinding
{
public:
    PViewRendererTestBinding(PView& target, const PGUIStyle& style, PGUIComponentID componentID)
        : PViewRendererTestBinding(target, style.GetRenderer<TRendererType>(componentID)) {}

    PViewRendererTestBinding(PView& target, Ptr<const TRendererType> renderer)
        : m_Target(target), m_Renderer(std::move(renderer))
    {
        if (m_Renderer == nullptr) {
            PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, "Test binding requires a component renderer");
        }
        m_Context = m_Renderer->Setup(m_Target);
    }

    ~PViewRendererTestBinding()
    {
        m_Renderer->Teardown(m_Target, ptr_raw_pointer_cast(m_Context));
    }

    PViewRendererTestBinding(const PViewRendererTestBinding&) = delete;
    PViewRendererTestBinding& operator=(const PViewRendererTestBinding&) = delete;

    const TRendererType* operator->() const { return ptr_raw_pointer_cast(m_Renderer); }
    PtrTarget* GetContext() const { return ptr_raw_pointer_cast(m_Context); }

private:
    PView& m_Target;
    Ptr<const TRendererType> m_Renderer;
    Ptr<PtrTarget> m_Context;
};
