// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <cstring>
#include <utility>

#include <App/Application.h>
#include <GUI/Style/GUIStyle.h>
#include <GUI/ViewFactory.h>
#include <GUI/Widgets/Button.h>
#include <GUI/Widgets/MVCListView.h>
#include <GUI/Widgets/TextView.h>
#include <System/AppDefinition.h>
#include <System/ExceptionHandling.h>

#include "GUIStyleVariantItemView.h"

namespace guistylepreferences
{

class GUIStylePreferencesView : public PView
{
public:
    GUIStylePreferencesView();
    virtual void OnStyleChanged() override;
    void LoadConfiguration();

private:
    void RequestStyles(size_t firstIndex);
    void ReceiveStyles(PErrorCode error, size_t firstIndex, size_t totalCount, const std::vector<PGUIStyleInfo>& styles);
    void ReceiveConfiguration(PErrorCode error, const PString& style, const PString& variant);
    void ReceiveStyleChange(PErrorCode error, const PString& style, const PString& variant);
    void SelectConfiguredStyle();
    void UpdateConfigurationStatus();
    void ApplySelection();
    void Close();
    void UpdateEnabledState();
    void ShowError(const PString& action, PErrorCode error);

    Ptr<PMVCListView> m_StyleList;
    Ptr<PButton> m_ApplyButton;
    Ptr<PTextView> m_StatusView;
    std::vector<PGUIStyleInfo> m_Styles;
    PString m_ConfiguredStyle;
    PString m_ConfiguredVariant;
    PErrorCode m_ConfigurationError = PErrorCode::Success;
    bool m_ConfigurationReceived = false;
    bool m_CatalogLoaded = false;
    bool m_Applying = false;
};

class GUIStylePreferencesApplication : public PApplication
{
public:
    GUIStylePreferencesApplication();
    virtual void ThreadStarted() override;
    virtual void QuitRequested() override;

private:
    Ptr<GUIStylePreferencesView> m_View;
};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

GUIStylePreferencesView::GUIStylePreferencesView() : PView("gui_style_preferences")
{
    PViewFactory::Get().LoadView(ptr_tmp_cast(this), "Preferences/GUIStylePreferencesView.xml");
    FindChild("styles", m_StyleList);
    FindChild("apply", m_ApplyButton);
    FindChild("status", m_StatusView);
    Ptr<PButton> closeButton = FindChild<PButton>("close");
    if (m_StyleList == nullptr || m_ApplyButton == nullptr || m_StatusView == nullptr || closeButton == nullptr) {
        PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, "Failed to load GUI style preferences layout");
    }
    m_StyleList->VFCreateItemWidget.Connect(this, []() { return ptr_new<GUIStyleVariantItemView>(); });
    m_StyleList->VFUpdateItemWidget.Connect(this, [](Ptr<PView> widget, Ptr<const PtrTarget> item, bool isSelected, bool isHighlighted)
    {
        ptr_static_cast<GUIStyleVariantItemView>(widget)->SetData(ptr_static_cast<const GUIStyleVariantItemData>(item), isSelected, isHighlighted);
    });
    m_StyleList->VFUpdateItemWidgetSelection.Connect(this, [](Ptr<PView> widget, bool isSelected, bool isHighlighted)
    {
        ptr_static_cast<GUIStyleVariantItemView>(widget)->UpdateSelection(isSelected, isHighlighted);
    });
    m_StyleList->SignalSelectionChanged.Connect(this, &GUIStylePreferencesView::UpdateEnabledState);
    m_ApplyButton->SignalActivated.Connect(this, &GUIStylePreferencesView::ApplySelection);
    closeButton->SignalActivated.Connect(this, &GUIStylePreferencesView::Close);
    GetApplication()->SignalGUIStyleChanged.Connect(this, &GUIStylePreferencesView::ReceiveStyleChange);
    OnStyleChanged();
    UpdateEnabledState();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void GUIStylePreferencesView::OnStyleChanged()
{
    PView::OnStyleChanged();
    SetBackgroundBrush(PGUIColorID::Background);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void GUIStylePreferencesView::LoadConfiguration()
{
    RequestStyles(0);
    try
    {
        GetApplication()->RequestGUIStyleConfiguration(
            [view = ptr_tmp_cast(this)](PErrorCode error, const PString& style, const PString& variant) { view->ReceiveConfiguration(error, style, variant); });
    }
    PERROR_CATCH([this](PErrorCode error) { ShowError("Read GUI style", error); });
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void GUIStylePreferencesView::RequestStyles(size_t firstIndex)
{
    try
    {
        GetApplication()->RequestGUIStyles(
            [view = ptr_tmp_cast(this)](PErrorCode error, size_t pageIndex, size_t totalCount, const std::vector<PGUIStyleInfo>& styles)
            {
                view->ReceiveStyles(error, pageIndex, totalCount, styles);
            },
            firstIndex);
    }
    PERROR_CATCH([this](PErrorCode error) { ShowError("List GUI styles", error); });
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void GUIStylePreferencesView::ReceiveStyles(PErrorCode error, size_t firstIndex, size_t totalCount, const std::vector<PGUIStyleInfo>& styles)
{
    if (error != PErrorCode::Success)
    {
        ShowError("List GUI styles", error);
        return;
    }
    if (firstIndex != m_Styles.size() || (styles.empty() && firstIndex < totalCount))
    {
        ShowError("List GUI styles", PErrorCode::INVAL);
        return;
    }
    m_Styles.insert(m_Styles.end(), styles.begin(), styles.end());
    const size_t nextIndex = firstIndex + styles.size();
    if (nextIndex < totalCount)
    {
        RequestStyles(nextIndex);
    }
    else
    {
        m_StyleList->ClearSelection();
        m_StyleList->Clear();
        for (const PGUIStyleInfo& style : m_Styles)
        {
            for (const PGUIStyleVariantInfo& variant : style.Variants)
            {
                Ptr<GUIStyleVariantItemData> item = ptr_new<GUIStyleVariantItemData>();
                item->StyleName = style.Name;
                item->Variant = variant;
                m_StyleList->AddItem(item);
            }
        }
        m_CatalogLoaded = true;
        SelectConfiguredStyle();
        UpdateConfigurationStatus();
        UpdateEnabledState();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void GUIStylePreferencesView::ReceiveConfiguration(PErrorCode error, const PString& style, const PString& variant)
{
    m_Applying = false;
    m_ConfiguredStyle = style;
    m_ConfiguredVariant = variant;
    m_ConfigurationError = error;
    m_ConfigurationReceived = true;
    SelectConfiguredStyle();
    UpdateConfigurationStatus();
    UpdateEnabledState();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void GUIStylePreferencesView::ReceiveStyleChange(PErrorCode error, const PString& style, const PString& variant)
{
    if (!m_Applying) {
        ReceiveConfiguration(error, style, variant);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void GUIStylePreferencesView::SelectConfiguredStyle()
{
    if (m_CatalogLoaded && m_ConfigurationReceived)
    {
        m_StyleList->ClearSelection();
        for (size_t index = 0; index < m_StyleList->GetItemCount(); ++index)
        {
            const Ptr<GUIStyleVariantItemData> item = m_StyleList->GetItemAt<GUIStyleVariantItemData>(index);
            if (item->StyleName == m_ConfiguredStyle && item->Variant.Name == m_ConfiguredVariant)
            {
                m_StyleList->SetItemSelection(index, true);
                break;
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void GUIStylePreferencesView::UpdateConfigurationStatus()
{
    if (m_ConfigurationReceived)
    {
        if (m_ConfigurationError != PErrorCode::Success)
        {
            ShowError("Change GUI style", m_ConfigurationError);
        }
        else if (m_CatalogLoaded)
        {
            PString displayName;
            for (const PGUIStyleInfo& style : m_Styles)
            {
                if (style.Name == m_ConfiguredStyle)
                {
                    for (const PGUIStyleVariantInfo& variant : style.Variants)
                    {
                        if (variant.Name == m_ConfiguredVariant)
                        {
                            displayName = variant.DisplayName;
                            break;
                        }
                    }
                    break;
                }
            }
            PApplication* application = GetApplication();
            if (displayName.empty()) {
                m_StatusView->SetText("The saved GUI style is no longer available.");
            } else if (application->GetGUIStyleName() == m_ConfiguredStyle && application->GetGUIStyleVariant() == m_ConfiguredVariant) {
                m_StatusView->SetText(PString::format_string("Current style: {}", displayName));
            } else {
                m_StatusView->SetText(PString::format_string("Saved {}, but this application could not load it.", displayName));
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void GUIStylePreferencesView::ApplySelection()
{
    const Ptr<GUIStyleVariantItemData> item = m_StyleList->GetItemAt<GUIStyleVariantItemData>(m_StyleList->GetFirstSelected());
    if (m_Applying || item == nullptr) {
        return;
    }
    m_Applying = true;
    UpdateEnabledState();
    m_StatusView->SetText("Applying GUI style...");
    try
    {
        GetApplication()->SetGUIStyleConfiguration(
            [view = ptr_tmp_cast(this)](PErrorCode error, const PString& style, const PString& variant) { view->ReceiveConfiguration(error, style, variant); },
            item->StyleName,
            item->Variant.Name);
    }
    PERROR_CATCH([this](PErrorCode error) { m_Applying = false; ShowError("Change GUI style", error); UpdateEnabledState(); });
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void GUIStylePreferencesView::Close()
{
    GetApplication()->GetPort().SendMessage(INVALID_HANDLE, std::to_underlying(PMessageID::QUIT), nullptr, 0);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void GUIStylePreferencesView::UpdateEnabledState()
{
    const bool enabled = m_CatalogLoaded && !m_Applying && m_StyleList->GetItemCount() != 0;
    m_StyleList->SetEnable(enabled);
    m_StyleList->SetHitMode(enabled ? PViewHitMode::HitTest : PViewHitMode::IgnoreRecursive);
    m_ApplyButton->SetEnable(enabled && m_StyleList->GetFirstSelected() != INVALID_INDEX);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void GUIStylePreferencesView::ShowError(const PString& action, PErrorCode error)
{
    m_StatusView->SetText(PString::format_string("{} failed: {}", action, strerror(std::to_underlying(error))));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

GUIStylePreferencesApplication::GUIStylePreferencesApplication() : PApplication("GUIStylePreferences")
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void GUIStylePreferencesApplication::ThreadStarted()
{
    m_View = ptr_new<GUIStylePreferencesView>();
    if (AddView(m_View, PViewDockType::DockedWindow)) {
        m_View->LoadConfiguration();
    } else {
        Stop();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void GUIStylePreferencesApplication::QuitRequested()
{
    if (m_View != nullptr)
    {
        RemoveView(m_View);
        m_View = nullptr;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int gui_style_preferences_main(int argc, char* argv[])
{
    GUIStylePreferencesApplication* application = new GUIStylePreferencesApplication();
    application->Adopt();
    return 0;
}

static PAppDefinition g_GUIStylePreferencesDefinition(
    "GUIStylePreferences",
    "Select the GUI style for all applications.",
    gui_style_preferences_main);

} // namespace guistylepreferences
