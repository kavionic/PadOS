// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <algorithm>
#include <cstring>

#include <App/Application.h>
#include <GUI/ViewFactory.h>
#include <GUI/Style/GUIStyle.h>
#include <GUI/Widgets/Button.h>
#include <GUI/Widgets/DropdownMenu.h>
#include <GUI/Widgets/TextView.h>
#include <System/AppDefinition.h>
#include <System/ExceptionHandling.h>

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
    void UpdateVariants();
    void ApplySelection();
    void Close();
    void UpdateEnabledState();
    void ShowError(const PString& action, PErrorCode error);

    Ptr<PDropdownMenu> m_StyleMenu;
    Ptr<PDropdownMenu> m_VariantMenu;
    Ptr<PButton> m_ApplyButton;
    Ptr<PTextView> m_StatusView;
    std::vector<PGUIStyleInfo> m_Styles;
    PString m_ConfiguredStyle;
    PString m_ConfiguredVariant;
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
    FindChild("style", m_StyleMenu);
    FindChild("variant", m_VariantMenu);
    FindChild("apply", m_ApplyButton);
    FindChild("status", m_StatusView);
    Ptr<PButton> closeButton = FindChild<PButton>("close");
    if (m_StyleMenu == nullptr || m_VariantMenu == nullptr || m_ApplyButton == nullptr || m_StatusView == nullptr || closeButton == nullptr) {
        PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, "Failed to load GUI style preferences layout");
    }
    m_StyleMenu->SignalSelectionChanged.Connect(this, &GUIStylePreferencesView::UpdateVariants);
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
        m_StyleMenu->Clear();
        for (const PGUIStyleInfo& style : m_Styles) {
            m_StyleMenu->AppendItem(style.Name);
        }
        m_CatalogLoaded = true;
        SelectConfiguredStyle();
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
    SelectConfiguredStyle();
    if (error == PErrorCode::Success)
    {
        PApplication* application = GetApplication();
        if (application->GetGUIStyleName() == style && application->GetGUIStyleVariant() == variant) {
            m_StatusView->SetText(PString::format_string("Current style: {} / {}", style, variant));
        } else {
            m_StatusView->SetText(PString::format_string("Saved {} / {}, but this application could not load it.", style, variant));
        }
    }
    else
    {
        ShowError("Change GUI style", error);
    }
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
    if (m_CatalogLoaded)
    {
        for (size_t index = 0; index < m_Styles.size(); ++index)
        {
            if (m_Styles[index].Name == m_ConfiguredStyle)
            {
                m_StyleMenu->SetSelection(index, false);
                break;
            }
        }
        UpdateVariants();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void GUIStylePreferencesView::UpdateVariants()
{
    const size_t styleIndex = m_StyleMenu->GetSelection();
    m_VariantMenu->Clear();
    if (styleIndex < m_Styles.size())
    {
        const PGUIStyleInfo& style = m_Styles[styleIndex];
        for (const PString& variant : style.Variants) {
            m_VariantMenu->AppendItem(variant);
        }
        size_t selectedVariant = 0;
        if (style.Name == m_ConfiguredStyle)
        {
            const auto variant = std::find(style.Variants.begin(), style.Variants.end(), m_ConfiguredVariant);
            if (variant != style.Variants.end()) {
                selectedVariant = size_t(variant - style.Variants.begin());
            }
        }
        if (!style.Variants.empty()) {
            m_VariantMenu->SetSelection(selectedVariant, false);
        }
    }
    UpdateEnabledState();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void GUIStylePreferencesView::ApplySelection()
{
    const size_t styleIndex = m_StyleMenu->GetSelection();
    const size_t variantIndex = m_VariantMenu->GetSelection();
    if (styleIndex >= m_Styles.size() || variantIndex >= m_Styles[styleIndex].Variants.size()) {
        return;
    }
    m_Applying = true;
    UpdateEnabledState();
    m_StatusView->SetText("Applying GUI style...");
    try
    {
        GetApplication()->SetGUIStyleConfiguration(
            [view = ptr_tmp_cast(this)](PErrorCode error, const PString& style, const PString& variant) { view->ReceiveConfiguration(error, style, variant); },
            m_Styles[styleIndex].Name,
            m_Styles[styleIndex].Variants[variantIndex]);
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
    const bool enabled = m_CatalogLoaded && !m_Applying && !m_Styles.empty();
    m_StyleMenu->SetEnable(enabled);
    m_VariantMenu->SetEnable(enabled && m_VariantMenu->GetItemCount() != 0);
    m_ApplyButton->SetEnable(enabled && m_VariantMenu->GetItemCount() != 0);
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
    "Select the GUI style and variant for all applications.",
    gui_style_preferences_main);

} // namespace guistylepreferences
