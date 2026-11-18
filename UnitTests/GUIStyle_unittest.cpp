// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <algorithm>
#include <array>
#include <unistd.h>
#include <gtest/gtest.h>
#include <ApplicationServer/GUIStyleProtocol.h>
#include <GUI/Style/GUIStyleFactory.h>
#include <GUI/Style/SliderStyleData.h>
#include <GUI/Styles/DefaultGUIStyle.h>
#include <GUI/Styles/SkinnableGUIStyle.h>
#include <GUI/Widgets/Slider.h>
#include <GUI/Widgets/TabView.h>
#include <Storage/File.h>
#include <Storage/Path.h>
#include <Storage/StandardPaths.h>
#include <System/ExceptionHandling.h>

#include "GUITest.h"

class PGUIStylePayloadTestStyle : public PDefaultGUIStyle
{
public:
    using PDefaultGUIStyle::PDefaultGUIStyle;

    virtual void SliderGetPreferredSize(const PView& target, const PSliderStyleData& data, PPoint* minSize, PPoint* maxSize) const override
    {
        PDefaultGUIStyle::SliderGetPreferredSize(target, data, minSize, maxSize);
        m_LastPayload = data.StyleData;
    }

    WeakPtr<PtrTarget> GetLastPayload() const { return m_LastPayload; }

private:
    mutable WeakPtr<PtrTarget> m_LastPayload;
};

TEST(GUIStyleFactory, ListsRegisteredClassesAndTheirVariants)
{
    PGUIStyleFactory factory;
    factory.RegisterStyle("Default", &PDefaultGUIStyle::GetVariants, &PDefaultGUIStyle::CreateStyle);
    factory.RegisterStyle("Skinned", &PSkinnableGUIStyle::GetVariants, &PSkinnableGUIStyle::CreateStyle);
    const std::vector<PGUIStyleInfo> styles = factory.GetStyles();
    ASSERT_EQ(styles.size(), 2u);
    EXPECT_EQ(styles[0].Name, "Default");
    EXPECT_EQ(styles[0].Variants, std::vector<PString>({"Normal"}));
    EXPECT_EQ(styles[1].Name, "Skinned");
    EXPECT_EQ(styles[1].Variants, PSkinnableGUIStyle::GetVariants());
    EXPECT_TRUE(factory.HasVariant("Skinned", "Dark"));
    EXPECT_FALSE(factory.HasVariant("Default", "Dark"));
    EXPECT_FALSE(factory.HasVariant("Missing", "Normal"));
    EXPECT_THROW(factory.GetVariants("Missing"), std::system_error);
    EXPECT_THROW(factory.RegisterStyle("Default", &PDefaultGUIStyle::GetVariants, &PDefaultGUIStyle::CreateStyle), std::system_error);
    EXPECT_THROW(factory.RegisterStyle(PString::zero, &PDefaultGUIStyle::GetVariants, &PDefaultGUIStyle::CreateStyle), std::system_error);
    EXPECT_THROW(factory.RegisterStyle("MissingVariants", nullptr, &PDefaultGUIStyle::CreateStyle), std::system_error);
    EXPECT_THROW(factory.RegisterStyle("MissingFactory", &PDefaultGUIStyle::GetVariants, nullptr), std::system_error);
}

TEST(GUIStyleFactory, DiscoversSkinVariantsInstalledAndRemovedAfterRegistration)
{
    PGUIStyleFactory factory;
    factory.RegisterStyle("Skinned", &PSkinnableGUIStyle::GetVariants, &PSkinnableGUIStyle::CreateStyle);
    const std::vector<PString> originalVariants = factory.GetVariants("Skinned");
    const PString variant = "GUIStyleRuntimeTest";
    const PPath variantPath(PStandardPaths::GetPath(PStandardPath::GUI, PString::format_string("Styles/Skinned/{}", variant)));
    PPath stylePath(variantPath);
    stylePath.Append("Style.xml");
    ASSERT_EQ(mkdir(variantPath.c_str(), S_IRWXU), 0);
    PScopeExit cleanup([&stylePath, &variantPath]() { unlink(stylePath.c_str()); EXPECT_EQ(rmdir(variantPath.c_str()), 0); });
    EXPECT_FALSE(factory.HasVariant("Skinned", variant));
    EXPECT_EQ(factory.GetVariants("Skinned"), originalVariants);

    PFile styleFile(stylePath.GetPath(), O_WRONLY | O_CREAT | O_EXCL);
    ASSERT_TRUE(styleFile.IsValid());
    ASSERT_TRUE(styleFile.Write(PString("<Style background_color=\"#000000\" />")));
    styleFile.Close();

    std::vector<PString> expectedVariants = originalVariants;
    expectedVariants.push_back(variant);
    std::sort(expectedVariants.begin(), expectedVariants.end());
    EXPECT_EQ(PSkinnableGUIStyle::GetVariants(), expectedVariants);
    EXPECT_EQ(factory.GetVariants("Skinned"), expectedVariants);
    EXPECT_TRUE(factory.HasVariant("Skinned", variant));
    const std::vector<PGUIStyleInfo> styles = factory.GetStyles();
    ASSERT_EQ(styles.size(), 1u);
    EXPECT_EQ(styles.front().Variants, expectedVariants);

    ASSERT_EQ(unlink(stylePath.c_str()), 0);
    EXPECT_FALSE(factory.HasVariant("Skinned", variant));
    EXPECT_EQ(factory.GetVariants("Skinned"), originalVariants);
    EXPECT_EQ(factory.GetStyles().front().Variants, originalVariants);
}

TEST(GUIStyleProtocol, CopiesNamesAndVariantsAndRejectsTruncation)
{
    PGUIStyleInfo original{"Style \xc3\x85", {"Normal", "Dark"}};
    const PGUIStyleInfo expected = original;
    alignas(uint32_t) std::array<uint8_t, 256> buffer{};
    const size_t length = PArgumentPacker<PGUIStyleInfo>::GetSize(original);
    ASSERT_LE(length, buffer.size());
    ASSERT_EQ(PArgumentPacker<PGUIStyleInfo>::Write(original, buffer.data(), length), ssize_t(length));
    original.Name.clear();
    original.Variants.clear();
    PGUIStyleInfo restored;
    ASSERT_EQ(PArgumentPacker<PGUIStyleInfo>::Read(buffer.data(), length, &restored), ssize_t(length));
    EXPECT_EQ(restored.Name, expected.Name);
    EXPECT_EQ(restored.Variants, expected.Variants);
    for (size_t truncatedLength = 0; truncatedLength < length; ++truncatedLength) {
        EXPECT_EQ(PArgumentPacker<PGUIStyleInfo>::Read(buffer.data(), truncatedLength, &restored), -1);
    }
}

TEST(GUIStyleFactory, ConstructsEachVariantWithApplicationOwnedResources)
{
    RunGUITest([](PApplication& application)
    {
        const PGUIStyleFactory& factory = PGUIStyleFactory::Get();
        for (const PGUIStyleInfo& definition : factory.GetStyles())
        {
            for (const PString& variant : definition.Variants)
            {
                const Ptr<const PGUIStyle> style = factory.CreateStyle(definition.Name, variant, &application);
                ASSERT_NE(style, nullptr);
                ASSERT_NE(style->GetBackgroundBrush(), nullptr);
                EXPECT_EQ(style->GetBackgroundBrush()->GetApplication(), &application);
            }
        }
        EXPECT_THROW(factory.CreateStyle("Skinned", "Missing", &application), std::system_error);
        EXPECT_THROW(factory.CreateStyle("Missing", "Normal", &application), std::system_error);
        EXPECT_THROW(factory.CreateStyle("Default", "Normal", nullptr), std::system_error);
    });
}

TEST(GUIStyle, AttachedWidgetsReleasePreviousStylePayloadsInOnStyleChanged)
{
    RunGUITest([](PApplication& application)
    {
        const Ptr<PGUIStylePayloadTestStyle> firstStyle = ptr_new<PGUIStylePayloadTestStyle>(&application);
        application.SetGUIStyle(firstStyle);
        const Ptr<PSlider> slider = ptr_new<PSlider>("style_switch_slider");
        slider->SetFrame(PRect::FromSize(240.0f, 80.0f));
        slider->Hide();
        ASSERT_TRUE(application.AddView(slider, PViewDockType::TopLevelView));
        PScopeExit removeSlider([&application, &slider]() { application.RemoveView(slider); });
        slider->PreferredSizeChanged();
        const WeakPtr<PtrTarget> previousPayload = firstStyle->GetLastPayload();
        ASSERT_NE(previousPayload.Lock(), nullptr);
        const Ptr<PGUIStylePayloadTestStyle> secondStyle = ptr_new<PGUIStylePayloadTestStyle>(&application);
        application.SetGUIStyle(secondStyle);
        EXPECT_EQ(previousPayload.Lock(), nullptr);
        EXPECT_NE(secondStyle->GetLastPayload().Lock(), nullptr);
        EXPECT_EQ(slider->GetStyle(), Ptr<const PGUIStyle>(secondStyle));
        application.Idle();
        application.Sync();
    });
}

class PGUIStyleNotificationTestView : public PView
{
public:
    using PView::PView;

    virtual void OnStyleChanged() override
    {
        PView::OnStyleChanged();
        SignalStyleChanged();
    }

    Signal<void()> SignalStyleChanged;
};

TEST(GUIStyle, OnlyRegisteredRootsAndTheirChildrenReceiveStyleChanges)
{
    RunGUITest([](PApplication& application)
    {
        std::vector<PString> notifications;
        const Ptr<PGUIStyleNotificationTestView> root = ptr_new<PGUIStyleNotificationTestView>("style_root");
        const Ptr<PGUIStyleNotificationTestView> child = ptr_new<PGUIStyleNotificationTestView>("style_child");
        const Ptr<PGUIStyleNotificationTestView> handlerOnly = ptr_new<PGUIStyleNotificationTestView>("style_handler_only");
        root->AddChild(child);
        root->Hide();
        root->SignalStyleChanged.Connect([&notifications]() { notifications.emplace_back("root"); });
        child->SignalStyleChanged.Connect([&notifications]() { notifications.emplace_back("child"); });
        handlerOnly->SignalStyleChanged.Connect([&notifications]() { notifications.emplace_back("handler_only"); });
        ASSERT_TRUE(application.AddHandler(handlerOnly));
        PScopeExit removeHandler([&application, &handlerOnly]() { application.RemoveHandler(handlerOnly); });
        ASSERT_TRUE(application.AddView(root, PViewDockType::RootLevelView));
        PScopeExit removeRoot([&application, &root]()
        {
            if (root->HasFlags(PViewFlag::IsAttachedToScreen)) {
                application.RemoveView(root);
            } else if (root->GetLooper() == &application) {
                application.RemoveHandler(root);
            }
        });

        notifications.clear();
        application.SetGUIStyle(nullptr);
        EXPECT_EQ(notifications, std::vector<PString>({"root", "child"}));

        ASSERT_TRUE(application.RemoveView(root));
        ASSERT_TRUE(application.AddHandler(root));
        notifications.clear();
        application.SetGUIStyle(nullptr);
        EXPECT_TRUE(notifications.empty());

        ASSERT_TRUE(application.RemoveHandler(root));
        ASSERT_TRUE(application.AddView(root, PViewDockType::RootLevelView));
        notifications.clear();
        application.SetGUIStyle(nullptr);
        EXPECT_EQ(notifications, std::vector<PString>({"root", "child"}));
        application.Sync();
    });
}

TEST(GUIStyle, StyleCallbacksCanCloseAnotherRootWithoutKeepingItAlive)
{
    RunGUITest([](PApplication& application)
    {
        std::vector<PString> notifications;
        const Ptr<PGUIStyleNotificationTestView> firstRoot = ptr_new<PGUIStyleNotificationTestView>("first_style_root");
        Ptr<PGUIStyleNotificationTestView> removedRoot = ptr_new<PGUIStyleNotificationTestView>("removed_style_root");
        const Ptr<PGUIStyleNotificationTestView> lastRoot = ptr_new<PGUIStyleNotificationTestView>("last_style_root");
        const WeakPtr<PGUIStyleNotificationTestView> weakRemovedRoot = removedRoot;
        firstRoot->Hide();
        removedRoot->Hide();
        lastRoot->Hide();
        ASSERT_TRUE(application.AddView(firstRoot, PViewDockType::RootLevelView));
        PScopeExit removeFirstRoot([&application, &firstRoot]() { application.RemoveView(firstRoot); });
        ASSERT_TRUE(application.AddView(removedRoot, PViewDockType::RootLevelView));
        PScopeExit removeOtherRoot([&application, &removedRoot]()
        {
            if (removedRoot != nullptr) {
                application.RemoveView(removedRoot);
            }
        });
        ASSERT_TRUE(application.AddView(lastRoot, PViewDockType::RootLevelView));
        PScopeExit removeLastRoot([&application, &lastRoot]() { application.RemoveView(lastRoot); });
        application.Idle();

        firstRoot->SignalStyleChanged.Connect([&application, &removedRoot, &weakRemovedRoot, &notifications]()
        {
            notifications.emplace_back("first");
            if (removedRoot != nullptr)
            {
                EXPECT_TRUE(application.RemoveView(removedRoot));
                removedRoot = nullptr;
                EXPECT_EQ(weakRemovedRoot.Lock(), nullptr);
            }
        });
        removedRoot->SignalStyleChanged.Connect([&notifications]() { notifications.emplace_back("removed"); });
        lastRoot->SignalStyleChanged.Connect([&notifications]() { notifications.emplace_back("last"); });

        application.SetGUIStyle(nullptr);
        EXPECT_EQ(notifications, std::vector<PString>({"first", "last"}));
        EXPECT_EQ(weakRemovedRoot.Lock(), nullptr);
        application.Sync();
    });
}

TEST(GUIStyle, RemovingSelfAndQueuedSiblingsAdvancesCursorsAtEveryDepth)
{
    RunGUITest([](PApplication& application)
    {
        std::vector<PString> notifications;
        const Ptr<PGUIStyleNotificationTestView> root = ptr_new<PGUIStyleNotificationTestView>("cursor_root");
        const Ptr<PGUIStyleNotificationTestView> branch = ptr_new<PGUIStyleNotificationTestView>("cursor_branch");
        Ptr<PGUIStyleNotificationTestView> active = ptr_new<PGUIStyleNotificationTestView>("cursor_active");
        Ptr<PGUIStyleNotificationTestView> nextChild = ptr_new<PGUIStyleNotificationTestView>("cursor_next_child");
        Ptr<PGUIStyleNotificationTestView> nextSibling = ptr_new<PGUIStyleNotificationTestView>("cursor_next_sibling");
        const Ptr<PGUIStyleNotificationTestView> lastChild = ptr_new<PGUIStyleNotificationTestView>("cursor_last_child");
        const Ptr<PGUIStyleNotificationTestView> lastSibling = ptr_new<PGUIStyleNotificationTestView>("cursor_last_sibling");
        const WeakPtr<PView> weakActive = active;
        const WeakPtr<PView> weakNextChild = nextChild;
        const WeakPtr<PView> weakNextSibling = nextSibling;
        branch->AddChild(active);
        branch->AddChild(nextChild);
        branch->AddChild(lastChild);
        root->AddChild(branch);
        root->AddChild(nextSibling);
        root->AddChild(lastSibling);
        root->Hide();
        ASSERT_TRUE(application.AddView(root, PViewDockType::RootLevelView));
        PScopeExit removeRoot([&application, &root]() { application.RemoveView(root); });
        application.Idle();
        SignalTarget callbacks;

        active->SignalStyleChanged.Connect(
            &callbacks,
            [&notifications, &root, &branch, &active, &nextChild, &nextSibling, &weakActive, &weakNextChild, &weakNextSibling]()
            {
                notifications.emplace_back("active");
                root->RemoveChild(nextSibling);
                nextSibling = nullptr;
                EXPECT_EQ(weakNextSibling.Lock(), nullptr);
                branch->RemoveChild(nextChild);
                nextChild = nullptr;
                EXPECT_EQ(weakNextChild.Lock(), nullptr);
                EXPECT_TRUE(active->RemoveThis());
                active = nullptr;
                EXPECT_NE(weakActive.Lock(), nullptr);
            });
        nextChild->SignalStyleChanged.Connect(&callbacks, [&notifications]() { notifications.emplace_back("removed_child"); });
        nextSibling->SignalStyleChanged.Connect(&callbacks, [&notifications]() { notifications.emplace_back("removed_sibling"); });
        lastChild->SignalStyleChanged.Connect(&callbacks, [&notifications]() { notifications.emplace_back("last_child"); });
        lastSibling->SignalStyleChanged.Connect(&callbacks, [&notifications]() { notifications.emplace_back("last_sibling"); });

        application.SetGUIStyle(nullptr);
        EXPECT_EQ(notifications, std::vector<PString>({"active", "last_child", "last_sibling"}));
        EXPECT_EQ(weakActive.Lock(), nullptr);
        notifications.clear();
        application.SetGUIStyle(nullptr);
        EXPECT_EQ(notifications, std::vector<PString>({"last_child", "last_sibling"}));
        application.Sync();
    });
}

TEST(GUIStyle, QueuedViewsCanBeDetachedDirectlyOrUnlinkedAsEavesdroppers)
{
    RunGUITest([](PApplication& application)
    {
        for (bool eavesdropper : {false, true})
        {
            SCOPED_TRACE(eavesdropper);
            std::vector<PString> notifications;
            const Ptr<PGUIStyleNotificationTestView> root = ptr_new<PGUIStyleNotificationTestView>("removal_root");
            const Ptr<PGUIStyleNotificationTestView> first = ptr_new<PGUIStyleNotificationTestView>("removal_first");
            const Ptr<PGUIStyleNotificationTestView> removed = ptr_new<PGUIStyleNotificationTestView>("removal_queued");
            const Ptr<PGUIStyleNotificationTestView> last = ptr_new<PGUIStyleNotificationTestView>("removal_last");
            if (eavesdropper) {
                removed->MergeFlags(PViewFlag::Eavesdropper);
            }
            root->AddChild(first);
            root->AddChild(removed);
            root->AddChild(last);
            root->Hide();
            ASSERT_TRUE(application.AddView(root, PViewDockType::RootLevelView));
            PScopeExit removeRoot([&application, &root]() { application.RemoveView(root); });
            PScopeExit removeDetached([&application, &removed]()
            {
                if (removed->GetParent() == nullptr && removed->GetLooper() == &application) {
                    application.RemoveView(removed);
                }
            });
            application.Idle();
            SignalTarget callbacks;
            first->SignalStyleChanged.Connect(&callbacks, [&application, &root, &removed, &notifications, eavesdropper]()
            {
                notifications.emplace_back("first");
                if (eavesdropper)
                {
                    root->RemoveChild(removed);
                    EXPECT_EQ(removed->GetLooper(), &application);
                }
                else
                {
                    EXPECT_TRUE(application.RemoveView(removed));
                    EXPECT_EQ(removed->GetParent(), root);
                }
            });
            removed->SignalStyleChanged.Connect(&callbacks, [&notifications]() { notifications.emplace_back("removed"); });
            last->SignalStyleChanged.Connect(&callbacks, [&notifications]() { notifications.emplace_back("last"); });

            application.SetGUIStyle(nullptr);
            EXPECT_EQ(notifications, std::vector<PString>({"first", "last"}));
            if (!eavesdropper) {
                root->RemoveChild(removed);
            }
            application.Sync();
        }
    });
}

TEST(GUIStyle, AddingSiblingsDuringNotificationPreservesTraversal)
{
    RunGUITest([](PApplication& application)
    {
        size_t firstCount = 0;
        size_t lastCount = 0;
        size_t insertedCount = 0;
        size_t appendedCount = 0;
        bool added = false;
        const Ptr<PGUIStyleNotificationTestView> root = ptr_new<PGUIStyleNotificationTestView>("insert_root");
        const Ptr<PGUIStyleNotificationTestView> first = ptr_new<PGUIStyleNotificationTestView>("insert_first");
        const Ptr<PGUIStyleNotificationTestView> last = ptr_new<PGUIStyleNotificationTestView>("insert_last");
        const Ptr<PGUIStyleNotificationTestView> inserted = ptr_new<PGUIStyleNotificationTestView>("insert_before_cursor");
        const Ptr<PGUIStyleNotificationTestView> appended = ptr_new<PGUIStyleNotificationTestView>("insert_at_end");
        root->AddChild(first);
        root->AddChild(last);
        root->Hide();
        ASSERT_TRUE(application.AddView(root, PViewDockType::RootLevelView));
        PScopeExit removeRoot([&application, &root]() { application.RemoveView(root); });
        application.Idle();
        SignalTarget callbacks;
        first->SignalStyleChanged.Connect(&callbacks, [&firstCount, &added, &root, &inserted, &appended]()
        {
            ++firstCount;
            if (!added)
            {
                added = true;
                root->InsertChild(inserted, 1);
                root->AddChild(appended);
            }
        });
        last->SignalStyleChanged.Connect(&callbacks, [&lastCount]() { ++lastCount; });
        inserted->SignalStyleChanged.Connect(&callbacks, [&insertedCount]() { ++insertedCount; });
        appended->SignalStyleChanged.Connect(&callbacks, [&appendedCount]() { ++appendedCount; });

        application.SetGUIStyle(nullptr);
        EXPECT_EQ(firstCount, 1u);
        EXPECT_EQ(lastCount, 1u);
        EXPECT_GE(insertedCount, 1u);
        EXPECT_GE(appendedCount, 1u);
        EXPECT_EQ(root->GetChildAt(1), inserted);
        EXPECT_EQ(root->GetChildAt(3), appended);
        EXPECT_EQ(root->GetChildAt(4), nullptr);

        firstCount = 0;
        lastCount = 0;
        insertedCount = 0;
        appendedCount = 0;
        application.SetGUIStyle(nullptr);
        EXPECT_EQ(firstCount, 1u);
        EXPECT_EQ(lastCount, 1u);
        EXPECT_EQ(insertedCount, 1u);
        EXPECT_EQ(appendedCount, 1u);
        application.Sync();
    });
}

class PGUIStyleDeferredTabTestView : public PTabView
{
public:
    using PTabView::PTabView;

    virtual void Layout(const PPoint& delta) override
    {
        ++m_LayoutCount;
        PTabView::Layout(delta);
    }

    size_t GetLayoutCount() const { return m_LayoutCount; }

private:
    size_t m_LayoutCount = 0;
};

TEST(GUIStyle, NestedTabsDeferLayoutUntilAllWidgetsHaveChangedStyles)
{
    RunGUITest([](PApplication& application)
    {
        const Ptr<PGUIStyleDeferredTabTestView> outerTabs = ptr_new<PGUIStyleDeferredTabTestView>("outer_tabs");
        const Ptr<PGUIStyleDeferredTabTestView> innerTabs = ptr_new<PGUIStyleDeferredTabTestView>("inner_tabs");
        const Ptr<PSlider> content = ptr_new<PSlider>("content");
        const Ptr<PSlider> topBar = ptr_new<PSlider>("top_bar");
        innerTabs->AppendTab("Inner", content);
        outerTabs->AppendTab("Outer", innerTabs);
        outerTabs->SetTopBarClientView(topBar);
        outerTabs->SetFrame(PRect::FromSize(640.0f, 400.0f));
        outerTabs->Hide();
        ASSERT_TRUE(application.AddView(outerTabs, PViewDockType::TopLevelView));
        PScopeExit removeTabs([&application, &outerTabs]() { application.RemoveView(outerTabs); });
        application.Idle();
        const std::array<PGUIStyleInfo, 3> selections = {{
            {"Skinned", {"Normal"}},
            {"Skinned", {"Dark"}},
            {"Default", {"Normal"}}
        }};
        for (const PGUIStyleInfo& selection : selections)
        {
            const size_t outerLayoutCount = outerTabs->GetLayoutCount();
            const size_t innerLayoutCount = innerTabs->GetLayoutCount();
            application.SetGUIStyle(PGUIStyleFactory::Get().CreateStyle(selection.Name, selection.Variants.front(), &application));
            EXPECT_EQ(outerTabs->GetLayoutCount(), outerLayoutCount);
            EXPECT_EQ(innerTabs->GetLayoutCount(), innerLayoutCount);
            application.Idle();
            EXPECT_GT(outerTabs->GetLayoutCount(), outerLayoutCount);
            EXPECT_GT(innerTabs->GetLayoutCount(), innerLayoutCount);
            EXPECT_EQ(innerTabs->GetFrame(), outerTabs->GetClientFrame());
            EXPECT_EQ(content->GetFrame(), innerTabs->GetClientFrame());
        }
        application.Sync();
    });
}

TEST(GUIStyle, ServerAPIsThrowOnLocalValidationErrors)
{
    RunGUITest([](PApplication& application)
    {
        EXPECT_THROW(application.RequestGUIStyles({}), std::system_error);
        EXPECT_THROW(application.RequestGUIStyleConfiguration({}), std::system_error);
        EXPECT_THROW(application.SetGUIStyleConfiguration({}, DEFAULT_GUI_STYLE_NAME, DEFAULT_GUI_STYLE_VARIANT), std::system_error);

        const PString oversizedStyle(PAPPSERVER_MSG_BUFFER_SIZE, 'x');
        const PApplication::GUIStyleConfigurationCallback callback = [](PErrorCode, const PString&, const PString&) noexcept {};
        EXPECT_THROW(application.SetGUIStyleConfiguration(callback, oversizedStyle, DEFAULT_GUI_STYLE_VARIANT), std::system_error);
    });
}

TEST(GUIStyle, ServerAPIsListStylesAndRejectInvalidSelections)
{
    RunGUITest([](PApplication& application)
    {
        std::vector<PGUIStyleInfo> catalog;
        size_t nextIndex = 0;
        size_t totalCount = 0;
        do
        {
            bool replied = false;
            const size_t previousIndex = nextIndex;
            ASSERT_NO_THROW(application.RequestGUIStyles(
                [&catalog, &nextIndex, &totalCount, &replied](
                    PErrorCode error,
                    size_t firstIndex,
                    size_t count,
                    const std::vector<PGUIStyleInfo>& styles)
                {
                    replied = true;
                    EXPECT_EQ(error, PErrorCode::Success);
                    EXPECT_EQ(firstIndex, nextIndex);
                    catalog.insert(catalog.end(), styles.begin(), styles.end());
                    totalCount = count;
                    nextIndex += styles.size();
                },
                nextIndex));
            ASSERT_TRUE(application.WaitForReply(INVALID_HANDLE, ASGetGUIStylesReply::GetID()));
            ASSERT_TRUE(replied);
            ASSERT_TRUE(nextIndex >= totalCount || nextIndex > previousIndex);
        } while (nextIndex < totalCount);
        const std::vector<PGUIStyleInfo> expected = PGUIStyleFactory::Get().GetStyles();
        ASSERT_EQ(catalog.size(), expected.size());
        for (size_t index = 0; index < expected.size(); ++index)
        {
            EXPECT_EQ(catalog[index].Name, expected[index].Name);
            EXPECT_EQ(catalog[index].Variants, expected[index].Variants);
        }

        PString selectedStyle;
        PString selectedVariant;
        bool replied = false;
        ASSERT_NO_THROW(application.RequestGUIStyleConfiguration(
            [&selectedStyle, &selectedVariant, &replied](PErrorCode error, const PString& style, const PString& variant)
            {
                replied = true;
                EXPECT_EQ(error, PErrorCode::Success);
                selectedStyle = style;
                selectedVariant = variant;
            }));
        ASSERT_TRUE(application.WaitForReply(INVALID_HANDLE, ASGUIStyleConfigurationReply::GetID()));
        ASSERT_TRUE(replied);
        replied = false;
        ASSERT_NO_THROW(application.SetGUIStyleConfiguration(
            [&selectedStyle, &selectedVariant, &replied](PErrorCode error, const PString& style, const PString& variant)
            {
                replied = true;
                EXPECT_EQ(error, PErrorCode::INVAL);
                EXPECT_EQ(style, selectedStyle);
                EXPECT_EQ(variant, selectedVariant);
            },
            selectedStyle,
            "Unregistered test variant"));
        ASSERT_TRUE(application.WaitForReply(INVALID_HANDLE, ASGUIStyleConfigurationReply::GetID()));
        EXPECT_TRUE(replied);
    });
}
