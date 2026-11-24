// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <GUI/Styles/DefaultGUIStyle.h>
#include <GUI/Styles/SkinnableGUIStyle.h>
#include <GUI/Styles/Skinned/ViewRendererTextBoxSkinned.h>
#include <GUI/ViewFactoryContext.h>
#include <GUI/Widgets/TextBox.h>
#include <Storage/Path.h>
#include <Storage/StandardPaths.h>
#include <System/ExceptionHandling.h>

#include "GUITest.h"

TEST(TextBoxStyle, EditorSizeChangesUpdateTheWidgetWithoutTextNotifications)
{
    RunGUITest([](PApplication& application)
    {
        Ptr<PTextBox> textBox = ptr_new<PTextBox>("text_box", "i");
        const Ptr<PTextEditView> editor = textBox->GetChildAt<PTextEditView>(size_t(0));
        ASSERT_NE(editor, nullptr);
        ASSERT_NE(textBox->GetRenderer(), nullptr);
        textBox->SetFrame(PRect(0.0f, 0.0f, 200.0f, 40.0f));
        textBox->Hide();
        ASSERT_TRUE(application.AddView(textBox, PViewDockType::TopLevelView));
        PScopeExit removeTextBox([&application, &textBox] { application.RemoveView(textBox); });
        EXPECT_EQ(editor->GetFrame(), PRect(4.0f, 4.0f, 196.0f, 36.0f));
        EXPECT_EQ(textBox->GetScrolledView(), editor);
        EXPECT_EQ(textBox->GetPreferredSize(PPrefSizeType::Smallest), editor->GetSizeForString("i") + PPoint(8.0f));
        size_t notifications = 0;
        textBox->SignalTextChanged.Connect([&notifications](const PString&, bool, PTextBox*) { ++notifications; });
        const PPoint initialSize = textBox->GetPreferredSize(PPrefSizeType::Smallest);
        const PString longerText = "A substantially longer text box value";
        textBox->SetText(longerText, false);
        EXPECT_EQ(notifications, 0u);
        EXPECT_GT(textBox->GetPreferredSize(PPrefSizeType::Smallest).x, initialSize.x);
        EXPECT_EQ(textBox->GetPreferredSize(PPrefSizeType::Smallest), textBox->GetSizeForString(longerText));
        editor->InsertText("More ", false);
        EXPECT_EQ(notifications, 0u);
        EXPECT_EQ(textBox->GetPreferredSize(PPrefSizeType::Smallest), textBox->GetSizeForString(textBox->GetText()));
        textBox->SetText("Changed");
        EXPECT_EQ(notifications, 1u);
        textBox->MergeFlags(PTextBoxFlag::IncludeLineGap);
        EXPECT_TRUE(editor->HasFlags(PTextBoxFlag::IncludeLineGap));
        EXPECT_EQ(textBox->GetPreferredSize(PPrefSizeType::Smallest), editor->GetSizeForString("Changed") + PPoint(8.0f));
        const Ptr<PFont> font = ptr_new<PFont>(PFontID::e_FontLarge);
        textBox->SetFont(font);
        EXPECT_EQ(editor->GetFont(), font);
        EXPECT_EQ(textBox->GetPreferredSize(PPrefSizeType::Smallest), textBox->GetSizeForString("Changed"));
        textBox->SetFrame(PRect(0.0f, 0.0f, 2.0f, 2.0f));
        EXPECT_EQ(editor->GetFrame().Size(), PPoint());
    });
}

TEST(TextBoxStyle, XmlFlagsAndEnabledStateAreReadyBeforeRendererSetup)
{
    RunGUITest([](PApplication& application)
    {
        pugi::xml_document document;
        pugi::xml_node xmlData = document.append_child("TextBox");
        xmlData.append_attribute("text") = "Read only";
        xmlData.append_attribute("enabled") = false;
        xmlData.append_attribute("flags") = "ReadOnly|RaisedFrame|IncludeLineGap";
        PViewFactoryContext factory;
        const Ptr<PTextBox> textBox = ptr_new<PTextBox>(factory, xmlData);
        const Ptr<PTextEditView> editor = textBox->GetChildAt<PTextEditView>(size_t(0));
        ASSERT_NE(editor, nullptr);
        ASSERT_NE(textBox->GetRenderer(), nullptr);
        EXPECT_TRUE(textBox->GetTextBoxStyleData().IsReadOnly);
        EXPECT_TRUE(textBox->GetTextBoxStyleData().HasRaisedFrame);
        EXPECT_FALSE(textBox->GetTextBoxStyleData().IsEnabled);
        EXPECT_FALSE(editor->IsEnabled());
        EXPECT_TRUE(editor->HasFlags(PTextBoxFlag::ReadOnly));
        EXPECT_TRUE(editor->HasFlags(PTextBoxFlag::IncludeLineGap));
        EXPECT_EQ(editor->GetColors()->DisabledTextColor, PColor(PNamedColors::dimgray));
        textBox->Hide();
        ASSERT_TRUE(application.AddView(textBox, PViewDockType::TopLevelView));
        PScopeExit removeTextBox([&application, &textBox] { application.RemoveView(textBox); });
        textBox->SetEnable(true);
        EXPECT_TRUE(editor->IsEnabled());
        EXPECT_EQ(editor->GetColors()->ReadOnlyBackgroundColor, PColor(PNamedColors::darkgray));
        textBox->ClearFlags(PTextBoxFlag::ReadOnly | PTextBoxFlag::RaisedFrame);
        EXPECT_FALSE(textBox->GetTextBoxStyleData().IsReadOnly);
        EXPECT_FALSE(textBox->GetTextBoxStyleData().HasRaisedFrame);
        EXPECT_EQ(editor->GetColors()->BackgroundColor, PColor(PNamedColors::white));
        EXPECT_EQ(textBox->GetPreferredSize(PPrefSizeType::Smallest), textBox->GetSizeForString("Read only"));
    });
}

TEST(TextBoxStyle, ThemeChangesUpdateGeometryAndEveryEditorColorState)
{
    RunGUITest([](PApplication& application)
    {
        const Ptr<const PGUIStyle> defaultStyle = ptr_tmp_cast(application.GetGUIStyle());
        const Ptr<PTextBox> textBox = ptr_new<PTextBox>("styled_text_box", "Text survives style changes");
        textBox->SetFrame(PRect(0.0f, 0.0f, 200.0f, 40.0f));
        textBox->Hide();
        ASSERT_TRUE(application.AddView(textBox, PViewDockType::TopLevelView));
        PScopeExit removeTextBox([&application, &textBox] { application.RemoveView(textBox); });
        const Ptr<PTextEditView> editor = textBox->GetChildAt<PTextEditView>(size_t(0));
        ASSERT_NE(editor, nullptr);
        const Ptr<const PTextEditColors> colors = editor->GetColors();
        for (const char* variant : {"Normal", "Dark"})
        {
            const PPath directory(PStandardPaths::GetPath(PStandardPath::GUI, PString::format_string("Styles/Skinned/{}", variant)));
            const Ptr<PSkinnableGUIStyle> style = ptr_new<PSkinnableGUIStyle>(directory, &application);
            application.SetGUIStyle(style);
            EXPECT_NE(dynamic_cast<const PViewRendererTextBoxSkinned*>(textBox->GetRenderer()), nullptr);
            EXPECT_EQ(editor->GetColors(), colors);
            EXPECT_EQ(editor->GetFrame(), PRect(8.0f, 6.0f, 192.0f, 34.0f));
            EXPECT_EQ(colors->BackgroundColor, style->GetColor(PGUIColorID::TextBoxBackgroundNormal));
            EXPECT_EQ(colors->TextColor, style->GetColor(PGUIColorID::TextBoxTextNormal));
            EXPECT_EQ(textBox->GetPreferredSize(PPrefSizeType::Smallest), textBox->GetSizeForString(textBox->GetText()));
            textBox->MergeFlags(PTextBoxFlag::ReadOnly);
            EXPECT_EQ(colors->ReadOnlyBackgroundColor, style->GetColor(PGUIColorID::TextBoxBackgroundReadOnly));
            EXPECT_EQ(colors->ReadOnlyTextColor, style->GetColor(PGUIColorID::TextBoxTextReadOnly));
            textBox->SetEnable(false);
            EXPECT_EQ(colors->DisabledBackgroundColor, style->GetColor(PGUIColorID::TextBoxBackgroundDisabled));
            EXPECT_EQ(colors->DisabledTextColor, style->GetColor(PGUIColorID::TextBoxTextDisabled));
            EXPECT_FALSE(editor->IsEnabled());
            textBox->SetEnable(true);
            textBox->ClearFlags(PTextBoxFlag::ReadOnly);
        }
        application.SetGUIStyle(defaultStyle);
        EXPECT_EQ(textBox->GetText(), "Text survives style changes");
        EXPECT_EQ(editor->GetFrame(), PRect(4.0f, 4.0f, 196.0f, 36.0f));
        EXPECT_EQ(colors->BackgroundColor, PColor(PNamedColors::white));
        EXPECT_EQ(colors->TextColor, PColor(PNamedColors::black));
    });
}
