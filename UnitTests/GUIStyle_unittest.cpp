// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <algorithm>
#include <array>
#include <stdexcept>
#include <type_traits>
#include <typeinfo>
#include <utility>

#include <gtest/gtest.h>
#include <unistd.h>

#include <ApplicationServer/GUIStyleProtocol.h>
#include <GUI/Style/GUIStyleFactory.h>
#include <GUI/Style/SliderStyleData.h>
#include <GUI/Style/ViewRenderer.h>
#include <GUI/Style/ViewRendererButton.h>
#include <GUI/Style/ViewRendererContext.h>
#include <GUI/Style/ViewRendererSlider.h>
#include <GUI/Style/ViewStyle.h>
#include <GUI/Style/ViewStyleSlider.h>
#include <GUI/Styles/Default/ViewRendererSliderDefault.h>
#include <GUI/Styles/DefaultGUIStyle.h>
#include <GUI/Styles/SkinnableGUIStyle.h>
#include <GUI/Widgets/Button.h>
#include <GUI/Widgets/CheckBox.h>
#include <GUI/Widgets/RadioButton.h>
#include <GUI/Widgets/Slider.h>
#include <GUI/Widgets/TabView.h>
#include <Storage/File.h>
#include <Storage/Path.h>
#include <Storage/StandardPaths.h>
#include <System/ExceptionHandling.h>

#include "GUITest.h"

struct PGUIRendererLifecycleTestState : PtrTarget
{
    WeakPtr<PtrTarget> LastContext;
    size_t SetupCount = 0;
    size_t PaintCount = 0;
    size_t TeardownCount = 0;
    size_t AttachmentCount = 0;
    size_t DetachmentCount = 0;
    size_t VisibilityChangeCount = 0;
    bool TeardownTargetValid = false;
    bool TeardownContextValid = false;
    bool Visible = true;
    bool FailPreferredSize = false;
    bool FailAttachment = false;
};

class PGUIStyleLifecycleTestRenderer : public PViewRendererSliderDefault
{
public:
    explicit PGUIStyleLifecycleTestRenderer(Ptr<PGUIRendererLifecycleTestState> state) : m_State(std::move(state)) {}

    virtual Ptr<PtrTarget> Setup(PView& target) const override
    {
        Ptr<PtrTarget> context = PViewRendererSliderDefault::Setup(target);
        m_State->LastContext = context;
        ++m_State->SetupCount;
        return context;
    }

    virtual void Teardown(PView& target, PtrTarget* context) const noexcept override
    {
        ++m_State->TeardownCount;
        m_State->TeardownTargetValid = dynamic_cast<const PViewStyleSlider*>(&target) != nullptr;
        m_State->TeardownContextValid = context != nullptr;
        PViewRendererSliderDefault::Teardown(target, context);
    }

    virtual void GetPreferredSize(const PView& target, PPoint* minSize, PPoint* maxSize, PtrTarget* context) const override
    {
        if (m_State->FailPreferredSize) {
            throw std::runtime_error("Renderer preferred-size failure");
        }
        PViewRendererSliderDefault::GetPreferredSize(target, minSize, maxSize, context);
    }

    virtual void AttachedToScreen(PView& target, PtrTarget* context) const override
    {
        ++m_State->AttachmentCount;
        if (m_State->FailAttachment) {
            throw std::runtime_error("Renderer attachment failure");
        }
    }

    virtual void DetachedFromScreen(PView& target, PtrTarget* context) const noexcept override
    {
        ++m_State->DetachmentCount;
    }

    virtual void VisibilityChanged(PView& target, bool visible, PtrTarget* context) const override
    {
        ++m_State->VisibilityChangeCount;
        m_State->Visible = visible;
    }

private:
    const Ptr<PGUIRendererLifecycleTestState> m_State;
};

class PGUIStyleGenericTestRenderer : public PViewRenderer
{
public:
    static constexpr PGUIComponentID COMPONENT_ID = PGUIComponentID::Button;

    explicit PGUIStyleGenericTestRenderer(Ptr<PGUIRendererLifecycleTestState> state) : m_State(std::move(state)) {}

    virtual Ptr<PtrTarget> Setup(PView& target) const override
    {
        Ptr<PtrTarget> context = ptr_new<PViewRendererContext<PViewStyleBase>>(target);
        m_State->LastContext = context;
        ++m_State->SetupCount;
        return context;
    }

    virtual void Teardown(PView& target, PtrTarget* context) const noexcept override
    {
        ++m_State->TeardownCount;
        m_State->TeardownTargetValid = dynamic_cast<const PViewStyleBase*>(&target) != nullptr;
        const PViewRendererContext<PViewStyleBase>* state = dynamic_cast<const PViewRendererContext<PViewStyleBase>*>(context);
        m_State->TeardownContextValid = state != nullptr && &state->GetView() == &target;
    }

    virtual void Paint(PView& target, const PRect& frame, const PRect& updateRect, PtrTarget* context) const override
    {
        ++m_State->PaintCount;
    }

    virtual void GetPreferredSize(const PView& target, PPoint* minSize, PPoint* maxSize, PtrTarget* context) const override
    {
        *minSize = PPoint(80.0f, 24.0f);
        *maxSize = *minSize;
    }

    virtual void AttachedToScreen(PView& target, PtrTarget* context) const override
    {
        ++m_State->AttachmentCount;
    }

    virtual void DetachedFromScreen(PView& target, PtrTarget* context) const noexcept override
    {
        ++m_State->DetachmentCount;
    }

    virtual void VisibilityChanged(PView& target, bool visible, PtrTarget* context) const override
    {
        ++m_State->VisibilityChangeCount;
        m_State->Visible = visible;
    }

private:
    const Ptr<PGUIRendererLifecycleTestState> m_State;
};

class PGUIStyleGenericTestView : public PView, public PViewStyle<PGUIStyleGenericTestRenderer>
{
public:
    explicit PGUIStyleGenericTestView(bool initializeRenderer = true) : PView("generic_style", PViewFlag::WillDraw), PViewStyle(this)
    {
        if (initializeRenderer) {
            InitializeRenderer();
        }
    }

    virtual ~PGUIStyleGenericTestView() override
    {
        ReleaseRenderer();
    }

    using PViewStyle::InitializeRenderer;
    using PViewStyle::ReleaseRenderer;

    virtual void OnPaint(const PRect& updateRect) override
    {
        const PViewRenderer* renderer = GetRenderer();
        if (renderer != nullptr) {
            renderer->Paint(*this, GetBounds(), updateRect, GetRendererContext());
        } else {
            PView::OnPaint(updateRect);
        }
    }

    virtual void CalculatePreferredSize(PPoint* minSize, PPoint* maxSize, bool includeWidth, bool includeHeight) override
    {
        const PViewRenderer* renderer = GetRenderer();
        if (renderer != nullptr) {
            renderer->GetPreferredSize(*this, minSize, maxSize, GetRendererContext());
        } else {
            PView::CalculatePreferredSize(minSize, maxSize, includeWidth, includeHeight);
        }
    }
};

class PGUIStylePayloadTestStyle : public PDefaultGUIStyle
{
public:
    explicit PGUIStylePayloadTestStyle(PApplication* application, bool failPreferredSize = false, bool failAttachment = false)
        : PDefaultGUIStyle(application), m_State(ptr_new<PGUIRendererLifecycleTestState>())
    {
        m_State->FailPreferredSize = failPreferredSize;
        m_State->FailAttachment = failAttachment;
        SetRenderer(PGUIComponentID::Slider, ptr_new<PGUIStyleLifecycleTestRenderer>(m_State));
    }

    WeakPtr<PtrTarget> GetLastPayload() const { return m_State->LastContext; }
    const PGUIRendererLifecycleTestState& GetState() const { return *m_State; }

private:
    const Ptr<PGUIRendererLifecycleTestState> m_State;
};

class PGUIStyleWrongRendererTestStyle : public PDefaultGUIStyle
{
public:
    explicit PGUIStyleWrongRendererTestStyle(PApplication* application) : PDefaultGUIStyle(application)
    {
        SetRenderer(PGUIComponentID::Slider, GetRenderer(PGUIComponentID::Button));
    }
};

class PGUIStyleCustomSliderTestView : public PView, public PViewStyleSlider
{
public:
    explicit PGUIStyleCustomSliderTestView(const PString& name) : PView(name), PViewStyleSlider(this)
    {
        InitializeRenderer();
    }

    virtual ~PGUIStyleCustomSliderTestView() override
    {
        ReleaseRenderer();
    }

    virtual const PSliderStyleData& GetSliderStyleData() const override { return m_Data; }

    virtual void CalculatePreferredSize(PPoint* minSize, PPoint* maxSize, bool includeWidth, bool includeHeight) override
    {
        const PViewRendererSlider* renderer = GetRenderer();
        if (renderer != nullptr) {
            renderer->GetPreferredSize(*this, minSize, maxSize, GetRendererContext());
        } else {
            PView::CalculatePreferredSize(minSize, maxSize, includeWidth, includeHeight);
        }
    }

private:
    PSliderStyleData m_Data;
};

class PGUIStyleMissingInterfaceTestView : public PView, public PViewStyle<PViewRendererSlider>
{
public:
    PGUIStyleMissingInterfaceTestView() : PView("missing_interface"), PViewStyle(this)
    {
        InitializeRenderer();
    }

    virtual ~PGUIStyleMissingInterfaceTestView() override
    {
        ReleaseRenderer();
    }
};

TEST(GUIStyleFactory, ListsRegisteredClassesAndTheirVariants)
{
    PGUIStyleFactory factory;
    factory.RegisterStyle("Default", &PDefaultGUIStyle::GetVariants, &PDefaultGUIStyle::CreateStyle);
    factory.RegisterStyle("Skinned", &PSkinnableGUIStyle::GetVariants, &PSkinnableGUIStyle::CreateStyle);
    const std::vector<PGUIStyleInfo> styles = factory.GetStyles();
    ASSERT_EQ(styles.size(), 2u);
    EXPECT_EQ(styles[0].Name, "Default");
    ASSERT_EQ(styles[0].Variants.size(), 1u);
    EXPECT_EQ(styles[0].Variants[0].Name, "Normal");
    EXPECT_EQ(styles[0].Variants[0].DisplayName, "Default");
    EXPECT_EQ(styles[0].Variants[0].PreviewPath, PStandardPaths::GetPath(PStandardPath::GUI, "Styles/Default/Normal/Preview.png"));
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
    const std::vector<PGUIStyleVariantInfo> originalVariants = factory.GetVariants("Skinned");
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
    ASSERT_TRUE(styleFile.Write(PString("<Style display_name=\"Runtime style\" preview=\"Preview.png\" color_background=\"#000000\" />")));
    styleFile.Close();

    std::vector<PGUIStyleVariantInfo> expectedVariants = originalVariants;
    PPath previewPath(variantPath);
    previewPath.Append("Preview.png");
    expectedVariants.push_back({variant, "Runtime style", previewPath.GetPath()});
    std::ranges::sort(expectedVariants, {}, &PGUIStyleVariantInfo::Name);
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
    PGUIStyleInfo original{"Style \xc3\x85", {{"Normal", "Light \xc3\xa6", "/GUI/Styles/Normal/Preview.png"}, {"Dark", "Dark", PString::zero}}};
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
            for (const PGUIStyleVariantInfo& variant : definition.Variants)
            {
                const Ptr<const PGUIStyle> style = factory.CreateStyle(definition.Name, variant.Name, &application);
                ASSERT_NE(style, nullptr);
                for (size_t index = 0; index < std::to_underlying(PGUIColorID::Count); ++index)
                {
                    const PGUIColorID colorID = PGUIColorID(index);
                    EXPECT_EQ(style->GetColor(colorID).GetAlpha(), 255);
                    const PBrush brush = style->GetBrush(colorID);
                    ASSERT_TRUE(brush.IsValid());
                    EXPECT_EQ(brush.GetApplication(), &application);
                    if (brush.GetType() == PBrushType::Solid) {
                        EXPECT_EQ(brush.GetColor(), style->GetColor(colorID));
                    }
                    EXPECT_EQ(style->GetBrush(colorID), brush);
                }
                EXPECT_THROW(style->GetBrush(PGUIColorID::Count), std::system_error);
            }
        }
        EXPECT_THROW(factory.CreateStyle("Skinned", "Missing", &application), std::system_error);
        EXPECT_THROW(factory.CreateStyle("Missing", "Normal", &application), std::system_error);
        EXPECT_THROW(factory.CreateStyle("Default", "Normal", nullptr), std::system_error);
    });
}

TEST(GUIStyle, PaletteBrushesRemainBoundToTheirOwningApplication)
{
    RunGUITest([](PApplication& currentApplication)
    {
        Ptr<PApplication> application = ptr_new<PApplication>("palette_brush_owner");
        const PDefaultGUIStyle style(ptr_raw_pointer_cast(application));
        EXPECT_EQ(style.GetBrush(PGUIColorID::Foreground).GetApplication(), application);
        EXPECT_EQ(style.GetBrush(PGUIColorID::Background).GetApplication(), application);
        application.Reset();
        EXPECT_EQ(PApplication::GetCurrentApplication(), &currentApplication);
        EXPECT_THROW(style.GetBrush(PGUIColorID::Foreground), std::system_error);
        EXPECT_THROW(style.GetBrush(PGUIColorID::Background), std::system_error);
    });
}

TEST(GUIStyle, DefaultPaletteAndBackgroundAreIndependentOfLegacyColors)
{
    RunGUITest([](PApplication& application)
    {
        const PColor original = pget_standard_color(PStandardColorID::DefaultBackground);
        PScopeExit restore([original] { pset_standard_color(PStandardColorID::DefaultBackground, original); });
        pset_standard_color(PStandardColorID::DefaultBackground, PColor(12, 34, 56));
        const PDefaultGUIStyle style(&application);
        EXPECT_EQ(style.GetColor(PGUIColorID::Foreground), PColor(0, 0, 0));
        EXPECT_EQ(style.GetColor(PGUIColorID::Background), PColor(169, 169, 169));
        const PBrush background = style.GetBrush(PGUIColorID::Background);
        ASSERT_TRUE(background.IsValid());
        EXPECT_EQ(background.GetApplication(), &application);
        EXPECT_EQ(background.GetType(), PBrushType::Solid);
        EXPECT_EQ(background.GetColor(), style.GetColor(PGUIColorID::Background));
        pset_standard_color(PStandardColorID::DefaultBackground, PColor(65, 43, 21));
        EXPECT_EQ(style.GetBrush(PGUIColorID::Background), background);
        EXPECT_EQ(background.GetColor(), style.GetColor(PGUIColorID::Background));
    });
}

TEST(GUIStyle, DefaultRenderersAreCachedForEveryComponent)
{
    const PDefaultGUIStyle style;
    for (size_t index = std::to_underlying(PGUIComponentID::Button); index < std::to_underlying(PGUIComponentID::Count); ++index)
    {
        const PGUIComponentID componentID = PGUIComponentID(index);
        const Ptr<const PViewRenderer> renderer = style.GetRenderer(componentID);
        ASSERT_NE(renderer, nullptr);
        EXPECT_EQ(style.GetRenderer(componentID), renderer);
    }
}

TEST(GUIStyle, ExplicitRenderersReplaceDefaultsAndClearingRestoresFallback)
{
    PDefaultGUIStyle style;
    const Ptr<const PViewRenderer> originalRenderer = style.GetRenderer(PGUIComponentID::Slider);
    const Ptr<const PViewRenderer> replacementRenderer = PGUIStyle::CreateDefaultRenderer(PGUIComponentID::Slider);
    ASSERT_NE(replacementRenderer, nullptr);
    EXPECT_NE(replacementRenderer, originalRenderer);
    style.SetRenderer(PGUIComponentID::Slider, replacementRenderer);
    EXPECT_EQ(style.GetRenderer<PViewRendererSlider>(PGUIComponentID::Slider), replacementRenderer);
    EXPECT_EQ(style.GetRenderer<PViewRendererButton>(PGUIComponentID::Slider), nullptr);

    style.SetRenderer(PGUIComponentID::Slider, nullptr);
    const Ptr<const PViewRendererSlider> restoredRenderer = style.GetRenderer<PViewRendererSlider>(PGUIComponentID::Slider);
    ASSERT_NE(restoredRenderer, nullptr);
    EXPECT_NE(restoredRenderer, originalRenderer);
    EXPECT_NE(restoredRenderer, replacementRenderer);
    EXPECT_EQ(style.GetRenderer(PGUIComponentID::Slider), restoredRenderer);
}

TEST(GUIStyle, UnsupportedComponentsDoNotCreateRenderers)
{
    PDefaultGUIStyle style;
    for (PGUIComponentID componentID : {PGUIComponentID::None, PGUIComponentID::Count, PGUIComponentID(0xff)})
    {
        EXPECT_EQ(PGUIStyle::CreateDefaultRenderer(componentID), nullptr);
        EXPECT_EQ(style.GetRenderer(componentID), nullptr);
        EXPECT_THROW(style.SetRenderer(componentID, nullptr), std::system_error);
    }
}

TEST(GUIStyle, AttachedWidgetsTeardownPreviousRendererBeforeRebinding)
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
        EXPECT_EQ(firstStyle->GetState().SetupCount, 1u);
        EXPECT_EQ(firstStyle->GetState().TeardownCount, 1u);
        EXPECT_TRUE(firstStyle->GetState().TeardownTargetValid);
        EXPECT_TRUE(firstStyle->GetState().TeardownContextValid);
        EXPECT_EQ(firstStyle->GetState().DetachmentCount, 1u);
        EXPECT_EQ(secondStyle->GetState().SetupCount, 1u);
        EXPECT_EQ(secondStyle->GetState().AttachmentCount, 1u);
        EXPECT_NE(secondStyle->GetLastPayload().Lock(), nullptr);
        EXPECT_EQ(slider->GetStyle(), Ptr<const PGUIStyle>(secondStyle));
        application.Idle();
        application.Sync();
    });
}

TEST(GUIStyle, DetachedWidgetsRefreshRendererBindingsWhenUsed)
{
    RunGUITest([](PApplication& application)
    {
        const Ptr<PGUIStylePayloadTestStyle> firstStyle = ptr_new<PGUIStylePayloadTestStyle>(&application);
        application.SetGUIStyle(firstStyle);
        const Ptr<PSlider> slider = ptr_new<PSlider>("detached_style_slider");
        slider->SetValue(0.75f, false);
        const WeakPtr<PtrTarget> previousContext = firstStyle->GetLastPayload();
        ASSERT_NE(previousContext.Lock(), nullptr);
        const Ptr<PGUIStylePayloadTestStyle> secondStyle = ptr_new<PGUIStylePayloadTestStyle>(&application);
        application.SetGUIStyle(secondStyle);
        EXPECT_EQ(firstStyle->GetState().TeardownCount, 0u);
        EXPECT_EQ(slider->GetRenderer(), secondStyle->GetRenderer<PViewRendererSlider>(PGUIComponentID::Slider));
        EXPECT_EQ(previousContext.Lock(), nullptr);
        EXPECT_EQ(firstStyle->GetState().TeardownCount, 1u);
        EXPECT_EQ(secondStyle->GetState().SetupCount, 1u);
        EXPECT_FLOAT_EQ(slider->GetValue(), 0.75f);
    });
}

TEST(GUIStyle, CustomTargetsShareRenderersWithoutSharingContextOrOwningTheView)
{
    RunGUITest([](PApplication& application)
    {
        const Ptr<PGUIStylePayloadTestStyle> style = ptr_new<PGUIStylePayloadTestStyle>(&application);
        application.SetGUIStyle(style);
        Ptr<PGUIStyleCustomSliderTestView> first = ptr_new<PGUIStyleCustomSliderTestView>("first_custom_slider");
        Ptr<PGUIStyleCustomSliderTestView> second = ptr_new<PGUIStyleCustomSliderTestView>("second_custom_slider");
        EXPECT_EQ(first->GetRenderer(), second->GetRenderer());
        EXPECT_NE(first->GetRendererContext(), second->GetRendererContext());
        first->PreferredSizeChanged();
        EXPECT_GT(first->GetPreferredSize(PPrefSizeType::Smallest).x, 0.0f);
        const WeakPtr<PView> firstView = first;
        const WeakPtr<PtrTarget> firstContext = ptr_tmp_cast(first->GetRendererContext());
        const WeakPtr<PtrTarget> secondContext = ptr_tmp_cast(second->GetRendererContext());
        first.Reset();
        EXPECT_EQ(firstView.Lock(), nullptr);
        EXPECT_EQ(firstContext.Lock(), nullptr);
        EXPECT_NE(secondContext.Lock(), nullptr);
        EXPECT_EQ(style->GetState().TeardownCount, 1u);
        EXPECT_TRUE(style->GetState().TeardownTargetValid);
        second.Reset();
        EXPECT_EQ(secondContext.Lock(), nullptr);
        EXPECT_EQ(style->GetState().SetupCount, 2u);
        EXPECT_EQ(style->GetState().TeardownCount, 2u);
    });
}

TEST(GUIStyle, GenericViewStyleMixinReceivesLifecycleCallbacksThroughPView)
{
    RunGUITest([](PApplication& application)
    {
        const Ptr<PGUIRendererLifecycleTestState> firstState = ptr_new<PGUIRendererLifecycleTestState>();
        const Ptr<PDefaultGUIStyle> firstStyle = ptr_new<PDefaultGUIStyle>(&application);
        firstStyle->SetRenderer(PGUIComponentID::Button, ptr_new<PGUIStyleGenericTestRenderer>(firstState));
        application.SetGUIStyle(firstStyle);
        Ptr<PGUIStyleGenericTestView> view = ptr_new<PGUIStyleGenericTestView>();
        const WeakPtr<PView> weakView = view;
        const WeakPtr<PtrTarget> firstContext = firstState->LastContext;
        view->SetFrame(PRect::FromSize(80.0f, 24.0f));
        view->OnPaint(view->GetBounds());
        EXPECT_EQ(firstState->PaintCount, 1u);
        view->MoveTo(PApplication::GetScreenFrame().BottomRight());
        view->Hide();
        ASSERT_TRUE(application.AddView(view, PViewDockType::TopLevelView));
        PScopeExit removeView([&application, &view]()
        {
            if (view != nullptr && view->HasFlags(PViewFlag::IsAttachedToScreen)) {
                application.RemoveView(view);
            }
        });
        EXPECT_EQ(firstState->AttachmentCount, 1u);
        EXPECT_EQ(view->GetPreferredSize(PPrefSizeType::Smallest), PPoint(80.0f, 24.0f));
        view->Show();
        EXPECT_TRUE(firstState->Visible);
        view->Hide();
        EXPECT_FALSE(firstState->Visible);
        EXPECT_EQ(firstState->VisibilityChangeCount, 3u);

        const Ptr<PGUIRendererLifecycleTestState> secondState = ptr_new<PGUIRendererLifecycleTestState>();
        const Ptr<PDefaultGUIStyle> secondStyle = ptr_new<PDefaultGUIStyle>(&application);
        secondStyle->SetRenderer(PGUIComponentID::Button, ptr_new<PGUIStyleGenericTestRenderer>(secondState));
        application.SetGUIStyle(secondStyle);
        EXPECT_EQ(firstContext.Lock(), nullptr);
        EXPECT_EQ(firstState->DetachmentCount, 1u);
        EXPECT_EQ(firstState->TeardownCount, 1u);
        EXPECT_TRUE(firstState->TeardownTargetValid);
        EXPECT_TRUE(firstState->TeardownContextValid);
        EXPECT_EQ(secondState->SetupCount, 1u);
        EXPECT_EQ(secondState->AttachmentCount, 1u);
        EXPECT_EQ(view->GetRenderer(), secondStyle->GetRenderer(PGUIComponentID::Button));

        ASSERT_TRUE(application.RemoveView(view));
        EXPECT_EQ(secondState->DetachmentCount, 1u);
        view.Reset();
        EXPECT_EQ(weakView.Lock(), nullptr);
        EXPECT_EQ(secondState->LastContext.Lock(), nullptr);
        EXPECT_EQ(secondState->TeardownCount, 1u);
        EXPECT_TRUE(secondState->TeardownTargetValid);
        EXPECT_TRUE(secondState->TeardownContextValid);
        application.Sync();
    });
}

TEST(GUIStyle, ViewReferenceBindingWaitsForInitializationAndCanBeReinitializedAfterRelease)
{
    RunGUITest([](PApplication& application)
    {
        const Ptr<PGUIRendererLifecycleTestState> state = ptr_new<PGUIRendererLifecycleTestState>();
        const Ptr<PDefaultGUIStyle> style = ptr_new<PDefaultGUIStyle>(&application);
        style->SetRenderer(PGUIComponentID::Button, ptr_new<PGUIStyleGenericTestRenderer>(state));
        application.SetGUIStyle(style);
        const Ptr<PGUIStyleGenericTestView> view = ptr_new<PGUIStyleGenericTestView>(false);

        for (size_t iteration = 0; iteration < 2; ++iteration)
        {
            view->OnStyleChanged();
            view->HandleScreenAttachmentChanged(true);
            view->OnVisibilityChanged(false);
            view->HandleScreenAttachmentChanged(false);
            EXPECT_EQ(view->GetRenderer(), nullptr);
            EXPECT_EQ(view->GetRendererContext(), nullptr);
            EXPECT_EQ(state->SetupCount, iteration);
            EXPECT_EQ(state->TeardownCount, iteration);
            EXPECT_EQ(state->AttachmentCount, 0u);
            EXPECT_EQ(state->DetachmentCount, 0u);
            EXPECT_EQ(state->VisibilityChangeCount, 0u);

            view->InitializeRenderer();
            EXPECT_EQ(state->SetupCount, iteration + 1);
            EXPECT_EQ(view->GetRenderer(), style->GetRenderer(PGUIComponentID::Button));
            ASSERT_NE(state->LastContext.Lock(), nullptr);
            view->ReleaseRenderer();
            EXPECT_EQ(state->LastContext.Lock(), nullptr);
            EXPECT_EQ(state->TeardownCount, iteration + 1);
            EXPECT_TRUE(state->TeardownTargetValid);
            EXPECT_TRUE(state->TeardownContextValid);
        }
    });
}

TEST(GUIStyle, RendererBindingRejectsIncompatibleRendererAndTargetInterfaces)
{
    RunGUITest([](PApplication& application)
    {
        const Ptr<PGUIStyleWrongRendererTestStyle> wrongStyle = ptr_new<PGUIStyleWrongRendererTestStyle>(&application);
        application.SetGUIStyle(wrongStyle);
        EXPECT_THROW(ptr_new<PSlider>("incompatible_renderer"), std::system_error);
        application.SetGUIStyle(ptr_new<PDefaultGUIStyle>(&application));
        EXPECT_THROW(ptr_new<PGUIStyleMissingInterfaceTestView>(), std::bad_cast);
    });
}

TEST(GUIStyle, ButtonWidgetsSelectTheirOwnRendererAndPreserveCommonButtonBehavior)
{
    static_assert(std::is_same_v<decltype(std::declval<const PButton&>().GetRenderer()), const PViewRendererButton*>);
    static_assert(std::is_same_v<decltype(std::declval<const PCheckBox&>().GetRenderer()), const PViewRendererCheckBox*>);
    static_assert(std::is_same_v<decltype(std::declval<const PRadioButton&>().GetRenderer()), const PViewRendererRadioButton*>);

    RunGUITest([](PApplication& application)
    {
        const std::array<PGUIStyleInfo, 2> selections = {{
            {"Default", {{"Normal", PString::zero, PString::zero}}},
            {"Skinned", {{"Normal", PString::zero, PString::zero}}}
        }};
        for (const PGUIStyleInfo& selection : selections)
        {
            SCOPED_TRACE(selection.Name.c_str());
            const Ptr<const PGUIStyle> style = PGUIStyleFactory::Get().CreateStyle(selection.Name, selection.Variants.front().Name, &application);
            application.SetGUIStyle(style);
            const Ptr<PButton> button = ptr_new<PButton>("typed_button", PString::zero);
            const Ptr<PCheckBox> checkBox = ptr_new<PCheckBox>("typed_check_box");
            const Ptr<PRadioButton> radioButton = ptr_new<PRadioButton>("typed_radio_button");
            EXPECT_EQ(button->GetRenderer(), style->GetRenderer(PGUIComponentID::Button));
            EXPECT_EQ(checkBox->GetRenderer(), style->GetRenderer(PGUIComponentID::CheckBox));
            EXPECT_EQ(radioButton->GetRenderer(), style->GetRenderer(PGUIComponentID::RadioButton));

            const std::array<Ptr<PButtonBase>, 3> buttons = {button, checkBox, radioButton};
            for (const Ptr<PButtonBase>& target : buttons)
            {
                target->SetLabel("A");
                const PPoint beforeSize = target->GetPreferredSize(PPrefSizeType::Smallest);
                const PtrTarget* context = target->GetRendererContext();
                ASSERT_NE(context, nullptr);
                target->SetLabel("A much longer button label");
                EXPECT_GT(target->GetPreferredSize(PPrefSizeType::Smallest).x, beforeSize.x);
                EXPECT_EQ(target->GetRendererContext(), context);
            }
        }
    });
}

TEST(GUIStyle, ButtonWidgetsRejectOtherButtonFamilyRenderers)
{
    RunGUITest([](PApplication& application)
    {
        const std::array<PGUIComponentID, 3> componentIDs = {PGUIComponentID::Button, PGUIComponentID::CheckBox, PGUIComponentID::RadioButton};
        for (PGUIComponentID requestedComponentID : componentIDs)
        {
            for (PGUIComponentID suppliedComponentID : componentIDs)
            {
                if (requestedComponentID != suppliedComponentID)
                {
                    SCOPED_TRACE(std::to_underlying(requestedComponentID));
                    SCOPED_TRACE(std::to_underlying(suppliedComponentID));
                    const Ptr<PDefaultGUIStyle> style = ptr_new<PDefaultGUIStyle>(&application);
                    style->SetRenderer(requestedComponentID, PGUIStyle::CreateDefaultRenderer(suppliedComponentID));
                    application.SetGUIStyle(style);
                    switch (requestedComponentID)
                    {
                        case PGUIComponentID::Button:
                            EXPECT_THROW(ptr_new<PButton>("wrong_button_renderer", "Button"), std::system_error);
                            break;
                        case PGUIComponentID::CheckBox:
                            EXPECT_THROW(ptr_new<PCheckBox>("wrong_check_box_renderer"), std::system_error);
                            break;
                        case PGUIComponentID::RadioButton:
                            EXPECT_THROW(ptr_new<PRadioButton>("wrong_radio_button_renderer"), std::system_error);
                            break;
                        default:
                            FAIL() << "Unexpected button component ID";
                            break;
                    }
                }
            }
        }
    });
}

TEST(GUIStyle, RendererInitializationDoesNotCalculatePreferredSize)
{
    RunGUITest([](PApplication& application)
    {
        const Ptr<PGUIStylePayloadTestStyle> style = ptr_new<PGUIStylePayloadTestStyle>(&application, true);
        application.SetGUIStyle(style);
        Ptr<PGUIStyleCustomSliderTestView> target = ptr_new<PGUIStyleCustomSliderTestView>("deferred_preferred_size");
        EXPECT_EQ(style->GetState().SetupCount, 1u);
        EXPECT_EQ(style->GetState().TeardownCount, 0u);
        ASSERT_NE(target->GetRendererContext(), nullptr);
        EXPECT_THROW(target->PreferredSizeChanged(), std::runtime_error);
        EXPECT_EQ(style->GetState().TeardownCount, 0u);
        target.Reset();
        EXPECT_EQ(style->GetState().TeardownCount, 1u);
        EXPECT_TRUE(style->GetState().TeardownTargetValid);
        EXPECT_TRUE(style->GetState().TeardownContextValid);
        EXPECT_EQ(style->GetLastPayload().Lock(), nullptr);
    });
}

TEST(GUIStyle, FailedRendererAttachmentReleasesThePartiallyActivatedContext)
{
    RunGUITest([](PApplication& application)
    {
        const Ptr<PGUIStylePayloadTestStyle> style = ptr_new<PGUIStylePayloadTestStyle>(&application, false, true);
        application.SetGUIStyle(style);
        const Ptr<PGUIStyleCustomSliderTestView> target = ptr_new<PGUIStyleCustomSliderTestView>("failed_attachment");
        const WeakPtr<PtrTarget> context = style->GetLastPayload();
        EXPECT_THROW(target->HandleScreenAttachmentChanged(true), std::runtime_error);
        EXPECT_EQ(style->GetState().AttachmentCount, 1u);
        EXPECT_EQ(style->GetState().DetachmentCount, 1u);
        EXPECT_EQ(style->GetState().TeardownCount, 1u);
        EXPECT_EQ(context.Lock(), nullptr);
        EXPECT_EQ(target->GetRendererContext(), nullptr);
        target->HandleScreenAttachmentChanged(false);
    });
}

TEST(GUIStyle, RendererActivationTracksAttachmentAndVisibilityWithoutRecreatingContext)
{
    RunGUITest([](PApplication& application)
    {
        const Ptr<PGUIStylePayloadTestStyle> style = ptr_new<PGUIStylePayloadTestStyle>(&application);
        application.SetGUIStyle(style);
        const Ptr<PSlider> slider = ptr_new<PSlider>("activation_slider");
        slider->SetFrame(PRect::FromSize(240.0f, 80.0f));
        slider->MoveTo(PApplication::GetScreenFrame().BottomRight());
        slider->Hide();
        const WeakPtr<PtrTarget> context = style->GetLastPayload();
        ASSERT_TRUE(application.AddView(slider, PViewDockType::TopLevelView));
        PScopeExit removeSlider([&application, &slider]
        {
            if (slider->HasFlags(PViewFlag::IsAttachedToScreen)) {
                application.RemoveView(slider);
            }
        });
        EXPECT_EQ(style->GetState().AttachmentCount, 1u);
        slider->Show();
        EXPECT_TRUE(style->GetState().Visible);
        slider->Hide();
        EXPECT_FALSE(style->GetState().Visible);
        EXPECT_EQ(style->GetState().VisibilityChangeCount, 3u);
        ASSERT_TRUE(application.RemoveView(slider));
        EXPECT_EQ(style->GetState().DetachmentCount, 1u);
        EXPECT_EQ(style->GetState().SetupCount, 1u);
        EXPECT_EQ(style->GetState().TeardownCount, 0u);
        EXPECT_NE(context.Lock(), nullptr);
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
            {"Skinned", {{"Normal", PString::zero, PString::zero}}},
            {"Skinned", {{"Dark", PString::zero, PString::zero}}},
            {"Default", {{"Normal", PString::zero, PString::zero}}}
        }};
        for (const PGUIStyleInfo& selection : selections)
        {
            const size_t outerLayoutCount = outerTabs->GetLayoutCount();
            const size_t innerLayoutCount = innerTabs->GetLayoutCount();
            application.SetGUIStyle(PGUIStyleFactory::Get().CreateStyle(selection.Name, selection.Variants.front().Name, &application));
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
