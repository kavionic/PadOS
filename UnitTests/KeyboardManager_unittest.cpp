// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <gtest/gtest.h>
#include <cstring>

#include <ApplicationServer/Protocol.h>
#include <System/ExceptionHandling.h>
#include <Threads/Looper.h>

#include <ApplicationServer/KeyboardManager.h>

class PKeyboardManagerTest : public testing::Test, public SignalTarget
{
public:
    static void SetUpTestSuite()
    {
        s_Configuration = ptr_new<SharedConfiguration>();
        s_Configuration->Manager.SetLayout("us", "", "", false);
    }

    static void TearDownTestSuite()
    {
        s_Configuration.Reset();
    }

protected:
    struct ReceivedKey
    {
        PKeyEvent Event;
        PString Text;
    };

    PKeyboardManagerTest()
        : m_Looper("keyboard_test", 4)
        , m_Manager(m_Looper, s_Configuration->Manager)
    {
        m_Manager.SignalKeyEvent.Connect(this, &PKeyboardManagerTest::Receive);
    }

    void SetUp() override
    {
        m_Manager.AddDevice(1);
        m_Manager.SetFocused(true);
    }

    void Send(PKeyCodes keyCode, bool pressed, int32_t sourceID = 1)
    {
        Send(m_Manager, keyCode, pressed, sourceID);
    }

    static void Send(PKeyboardManager& manager, PKeyCodes keyCode, bool pressed, int32_t sourceID = 1)
    {
        PRawKeyEvent event{};
        event.EventSize = sizeof(event);
        event.EventType = PInputEventType::RawKeyEvent;
        event.ClassID = PInputClass::Keyboard;
        event.EventID = pressed ? PInputEventID::KeyDown : PInputEventID::KeyUp;
        event.SourceID = sourceID;
        event.KeyCode = keyCode;
        manager.HandleKeyEvent(event);
    }

    bool WaitForRepeat()
    {
        const size_t firstEvent = m_Received.size();
        bool timedOut = false;
        PEventTimer timeout(TimeValNanos::FromSeconds(2.0), true);
        timeout.SignalTrigged.Connect([&timedOut](PEventTimer*) { timedOut = true; });

        CRITICAL_SCOPE(m_Looper.GetMutex());
        if (!m_Looper.AddTimer(&timeout, true)) {
            return false;
        }
        while (!timedOut)
        {
            if (!m_Looper.Tick()) {
                return false;
            }
            for (size_t index = firstEvent; index < m_Received.size(); ++index)
            {
                if ((m_Received[index].Event.Qualifiers & QUAL_REPEAT) != 0) {
                    return true;
                }
            }
        }
        return false;
    }

    void Receive(const PKeyEvent& event, const PString& text)
    {
        m_Received.push_back({event, text});
    }

    PLooper m_Looper;
    PKeyboardManager m_Manager;
    std::vector<ReceivedKey> m_Received;

private:
    struct SharedConfiguration : public PtrTarget
    {
        PLooper Looper{"keyboard_test_configuration", 4};
        PKeyboardManager Manager{Looper};
    };

    inline static Ptr<SharedConfiguration> s_Configuration;
};

TEST_F(PKeyboardManagerTest, ReusedConfigurationHasIndependentKeyState)
{
    Send(PKeyCodes::CAPS_LOCK, true);
    Send(PKeyCodes::CAPS_LOCK, false);
    Send(PKeyCodes::LEFT_SHIFT, true);
    Send(PKeyCodes::A, true);

    PKeyboardManager manager(m_Looper, m_Manager);
    manager.SignalKeyEvent.Connect([this](const PKeyEvent& event, const PString& text) { Receive(event, text); });
    manager.AddDevice(1);
    manager.SetFocused(true);
    const size_t firstEvent = m_Received.size();
    Send(manager, PKeyCodes::A, true);
    ASSERT_EQ(m_Received.size(), firstEvent + 1);
    ASSERT_EQ(m_Received.back().Event.KeyCode, PKeyCodes::A);
    EXPECT_EQ(m_Received.back().Event.KeySymbol, PKeySymbol(U'a'));
    EXPECT_EQ(m_Received.back().Text, "a");
    EXPECT_EQ(manager.GetQualifiers(), 0U);
    EXPECT_NE(m_Manager.GetQualifiers() & QUAL_CAPS_LOCK, 0U);
    EXPECT_NE(m_Manager.GetQualifiers() & QUAL_SHIFT, 0U);

    Send(manager, PKeyCodes::LEFT_SHIFT, true);
    Send(PKeyCodes::LEFT_SHIFT, false);
    Send(manager, PKeyCodes::A, false);
    Send(manager, PKeyCodes::A, true);
    EXPECT_EQ(m_Received.back().Text, "A");
    EXPECT_NE(manager.GetQualifiers() & QUAL_SHIFT, 0U);
    EXPECT_EQ(m_Manager.GetQualifiers() & QUAL_SHIFT, 0U);
}

TEST_F(PKeyboardManagerTest, ShiftCapsAndControl)
{
    Send(PKeyCodes::A, true);
    EXPECT_EQ(m_Received.back().Text, "a");
    EXPECT_EQ(m_Received.back().Event.KeySymbol, PKeySymbol(U'a'));
    Send(PKeyCodes::A, false);
    Send(PKeyCodes::CAPS_LOCK, true);
    Send(PKeyCodes::CAPS_LOCK, false);
    Send(PKeyCodes::A, true);
    EXPECT_EQ(m_Received.back().Text, "A");
    EXPECT_EQ(m_Received.back().Event.KeySymbol, PKeySymbol(U'A'));
    EXPECT_NE(m_Received.back().Event.Qualifiers & QUAL_CAPS_LOCK, 0U);
    Send(PKeyCodes::A, false);
    Send(PKeyCodes::LEFT_SHIFT, true);
    Send(PKeyCodes::A, true);
    EXPECT_EQ(m_Received.back().Text, "a");
    Send(PKeyCodes::A, false);
    Send(PKeyCodes::LEFT_CONTROL, true);
    Send(PKeyCodes::C, true);
    EXPECT_TRUE(m_Received.back().Text.empty());
    EXPECT_EQ(m_Received.back().Event.KeySymbol, PKeySymbol(U'c'));
    EXPECT_NE(m_Received.back().Event.Qualifiers & QUAL_CTRL, 0U);
}

TEST_F(PKeyboardManagerTest, LeftAndRightModifiersHaveDistinctSymbols)
{
    struct ModifierKey
    {
        PKeyCodes KeyCode;
        PKeySymbol KeySymbol;
    };
    static constexpr ModifierKey MODIFIERS[] =
    {
        {PKeyCodes::LEFT_SHIFT, PKeySymbol::LEFT_SHIFT},
        {PKeyCodes::RIGHT_SHIFT, PKeySymbol::RIGHT_SHIFT},
        {PKeyCodes::LEFT_CONTROL, PKeySymbol::LEFT_CONTROL},
        {PKeyCodes::RIGHT_CONTROL, PKeySymbol::RIGHT_CONTROL},
        {PKeyCodes::LEFT_ALT, PKeySymbol::LEFT_ALT},
        {PKeyCodes::RIGHT_ALT, PKeySymbol::RIGHT_ALT},
        {PKeyCodes::LEFT_GUI, PKeySymbol::LEFT_SUPER},
        {PKeyCodes::RIGHT_GUI, PKeySymbol::RIGHT_SUPER}
    };
    for (const ModifierKey& modifier : MODIFIERS)
    {
        const size_t firstEvent = m_Received.size();
        Send(modifier.KeyCode, true);
        Send(modifier.KeyCode, false);
        ASSERT_EQ(m_Received.size(), firstEvent + 2);
        EXPECT_EQ(m_Received[firstEvent].Event.KeyCode, modifier.KeyCode);
        EXPECT_EQ(m_Received[firstEvent].Event.KeySymbol, modifier.KeySymbol);
        EXPECT_EQ(m_Received.back().Event.KeyCode, modifier.KeyCode);
        EXPECT_EQ(m_Received.back().Event.KeySymbol, modifier.KeySymbol);
    }
}

TEST_F(PKeyboardManagerTest, RemappedModifierSidesSurviveLayoutChange)
{
    m_Manager.SetLayout("us", "", "altwin:meta_win", false);
    Send(PKeyCodes::LEFT_GUI, true);
    EXPECT_EQ(m_Received.back().Event.KeySymbol, PKeySymbol::LEFT_META);
    Send(PKeyCodes::RIGHT_GUI, true);
    EXPECT_EQ(m_Received.back().Event.KeySymbol, PKeySymbol::RIGHT_META);

    m_Manager.SetLayout("us", "", "altwin:hyper_win", false);
    Send(PKeyCodes::LEFT_GUI, false);
    EXPECT_EQ(m_Received.back().Event.KeyCode, PKeyCodes::LEFT_GUI);
    EXPECT_EQ(m_Received.back().Event.KeySymbol, PKeySymbol::LEFT_META);
    Send(PKeyCodes::RIGHT_GUI, false);
    EXPECT_EQ(m_Received.back().Event.KeyCode, PKeyCodes::RIGHT_GUI);
    EXPECT_EQ(m_Received.back().Event.KeySymbol, PKeySymbol::RIGHT_META);

    Send(PKeyCodes::LEFT_GUI, true);
    EXPECT_EQ(m_Received.back().Event.KeyCode, PKeyCodes::LEFT_GUI);
    EXPECT_EQ(m_Received.back().Event.KeySymbol, PKeySymbol::LEFT_HYPER);
    Send(PKeyCodes::LEFT_GUI, false);
    EXPECT_EQ(m_Received.back().Event.KeySymbol, PKeySymbol::LEFT_HYPER);
    Send(PKeyCodes::RIGHT_GUI, true);
    EXPECT_EQ(m_Received.back().Event.KeyCode, PKeyCodes::RIGHT_GUI);
    EXPECT_EQ(m_Received.back().Event.KeySymbol, PKeySymbol::RIGHT_HYPER);
    Send(PKeyCodes::RIGHT_GUI, false);
    EXPECT_EQ(m_Received.back().Event.KeySymbol, PKeySymbol::RIGHT_HYPER);
}

TEST_F(PKeyboardManagerTest, NorwegianLayoutAndHeldModifierSurviveLayoutChange)
{
    Send(PKeyCodes::LEFT_SHIFT, true);
    m_Manager.SetLayout("no", "", "", false);
    Send(PKeyCodes::LEFT_BRACKET, true);
    EXPECT_EQ(m_Received.back().Text, "\xc3\x85");
    EXPECT_EQ(m_Received.back().Event.KeySymbol, PKeySymbol(U'\u00c5'));
    EXPECT_EQ(m_Received.back().Event.KeyCode, PKeyCodes::LEFT_BRACKET);
    Send(PKeyCodes::LEFT_BRACKET, false);
    Send(PKeyCodes::LEFT_SHIFT, false);
    Send(PKeyCodes::APOSTROPHE, true);
    EXPECT_EQ(m_Received.back().Text, "\xc3\xa6");
}

TEST_F(PKeyboardManagerTest, GermanLayoutAndAltGr)
{
    m_Manager.SetLayout("de", "", "", false);
    Send(PKeyCodes::Y, true);
    EXPECT_EQ(m_Received.back().Text, "z");
    EXPECT_EQ(m_Received.back().Event.KeyCode, PKeyCodes::Y);
    EXPECT_EQ(m_Received.back().Event.KeySymbol, PKeySymbol(U'z'));
    Send(PKeyCodes::Y, false);
    Send(PKeyCodes::RIGHT_ALT, true);
    EXPECT_EQ(m_Received.back().Event.KeySymbol, PKeySymbol::ALT_GR);
    Send(PKeyCodes::E, true);
    EXPECT_EQ(m_Received.back().Text, "\xe2\x82\xac");
    EXPECT_EQ(m_Received.back().Event.KeySymbol, PKeySymbol(U'\u20ac'));
    EXPECT_NE(m_Received.back().Event.Qualifiers & QUAL_ALT_GR, 0U);
}

TEST_F(PKeyboardManagerTest, DeadKeyCompositionAndFocusReset)
{
    m_Manager.SetLayout("us", "intl", "", false);
    Send(PKeyCodes::APOSTROPHE, true);
    EXPECT_TRUE(m_Received.back().Text.empty());
    Send(PKeyCodes::APOSTROPHE, false);
    {
        PKeyboardManager manager(m_Looper, m_Manager);
        manager.SignalKeyEvent.Connect([this](const PKeyEvent& event, const PString& text) { Receive(event, text); });
        manager.AddDevice(1);
        manager.SetFocused(true);
        Send(manager, PKeyCodes::E, true);
        EXPECT_EQ(m_Received.back().Text, "e");
    }
    Send(PKeyCodes::E, true);
    EXPECT_EQ(m_Received.back().Text, "\xc3\xa9");
    EXPECT_EQ(m_Received.back().Event.KeySymbol, PKeySymbol(U'e'));
    Send(PKeyCodes::E, false);
    Send(PKeyCodes::APOSTROPHE, true);
    Send(PKeyCodes::APOSTROPHE, false);
    m_Manager.SetFocused(false);
    m_Manager.SetFocused(true);
    Send(PKeyCodes::E, true);
    EXPECT_EQ(m_Received.back().Text, "e");
}

TEST_F(PKeyboardManagerTest, DuplicateReportsAndKeyboardRemoval)
{
    m_Manager.AddDevice(2);
    Send(PKeyCodes::LEFT_SHIFT, true);
    const size_t count = m_Received.size();
    Send(PKeyCodes::LEFT_SHIFT, true);
    EXPECT_EQ(m_Received.size(), count);
    Send(PKeyCodes::LEFT_SHIFT, true, 2);
    m_Manager.RemoveDevice(1);
    Send(PKeyCodes::A, true, 2);
    EXPECT_EQ(m_Received.back().Text, "A");
    Send(PKeyCodes::A, false, 2);
    m_Manager.RemoveDevice(2);
    m_Manager.AddDevice(3);
    Send(PKeyCodes::A, true, 3);
    EXPECT_EQ(m_Received.back().Text, "a");
}

TEST_F(PKeyboardManagerTest, InvalidSelectionLeavesActiveLayoutUnchanged)
{
    EXPECT_THROW(m_Manager.SetLayout("no", "no_such_variant", "", false), std::system_error);
    EXPECT_EQ(m_Manager.GetLayout(), "us");
    Send(PKeyCodes::Y, true);
    EXPECT_EQ(m_Received.back().Text, "y");
}

TEST_F(PKeyboardManagerTest, PhysicalKeyAndPressSymbolSurviveLayoutChange)
{
    Send(PKeyCodes::Y, true);
    EXPECT_EQ(m_Received.back().Event.KeyCode, PKeyCodes::Y);
    EXPECT_EQ(m_Received.back().Event.KeySymbol, PKeySymbol(U'y'));
    m_Manager.SetLayout("de", "", "", false);
    Send(PKeyCodes::Y, false);
    EXPECT_EQ(m_Received.back().Event.KeyCode, PKeyCodes::Y);
    EXPECT_EQ(m_Received.back().Event.KeySymbol, PKeySymbol(U'y'));
    EXPECT_TRUE(m_Received.back().Text.empty());
    Send(PKeyCodes::Y, true);
    EXPECT_EQ(m_Received.back().Event.KeySymbol, PKeySymbol(U'z'));
    EXPECT_EQ(m_Received.back().Text, "z");
}

TEST_F(PKeyboardManagerTest, ReleaseKeepsPressSymbolAfterModifierChange)
{
    Send(PKeyCodes::LEFT_SHIFT, true);
    Send(PKeyCodes::NUM_1, true);
    EXPECT_EQ(m_Received.back().Event.KeySymbol, PKeySymbol(U'!'));
    Send(PKeyCodes::LEFT_SHIFT, false);
    Send(PKeyCodes::NUM_1, false);
    EXPECT_EQ(m_Received.back().Event.KeySymbol, PKeySymbol(U'!'));
    EXPECT_EQ(m_Received.back().Event.Qualifiers & QUAL_SHIFT, 0U);
    EXPECT_TRUE(m_Received.back().Text.empty());
    Send(PKeyCodes::NUM_1, true);
    EXPECT_EQ(m_Received.back().Event.KeySymbol, PKeySymbol(U'1'));
    EXPECT_EQ(m_Received.back().Text, "1");
}

TEST_F(PKeyboardManagerTest, EachDeviceRetainsItsPressSymbolThroughRemoval)
{
    m_Manager.AddDevice(2);
    Send(PKeyCodes::Y, true, 1);
    m_Manager.SetLayout("de", "", "", false);
    Send(PKeyCodes::Y, true, 2);
    EXPECT_EQ(m_Received.back().Event.KeySymbol, PKeySymbol(U'z'));
    Send(PKeyCodes::Y, false, 1);
    EXPECT_EQ(m_Received.back().Event.KeySymbol, PKeySymbol(U'y'));
    m_Manager.RemoveDevice(2);
    EXPECT_EQ(m_Received.back().Event.EventID, PInputEventID::KeyUp);
    EXPECT_EQ(m_Received.back().Event.SourceID, 2);
    EXPECT_EQ(m_Received.back().Event.KeyCode, PKeyCodes::Y);
    EXPECT_EQ(m_Received.back().Event.KeySymbol, PKeySymbol(U'z'));
    EXPECT_TRUE(m_Received.back().Text.empty());
}

TEST_F(PKeyboardManagerTest, RepeatKeepsPressSymbolWhileTextUsesCurrentModifiers)
{
    Send(PKeyCodes::A, true);
    EXPECT_EQ(m_Received.back().Event.KeySymbol, PKeySymbol(U'a'));
    Send(PKeyCodes::LEFT_SHIFT, true);
    ASSERT_TRUE(WaitForRepeat());
    EXPECT_EQ(m_Received.back().Event.KeyCode, PKeyCodes::A);
    EXPECT_EQ(m_Received.back().Event.KeySymbol, PKeySymbol(U'a'));
    EXPECT_EQ(m_Received.back().Text, "A");
    EXPECT_NE(m_Received.back().Event.Qualifiers & QUAL_SHIFT, 0U);
    EXPECT_NE(m_Received.back().Event.Qualifiers & QUAL_REPEAT, 0U);
    Send(PKeyCodes::LEFT_SHIFT, false);
    ASSERT_TRUE(WaitForRepeat());
    EXPECT_EQ(m_Received.back().Event.KeySymbol, PKeySymbol(U'a'));
    EXPECT_EQ(m_Received.back().Text, "a");
    EXPECT_EQ(m_Received.back().Event.Qualifiers & QUAL_SHIFT, 0U);
    Send(PKeyCodes::A, false);
    EXPECT_EQ(m_Received.back().Event.EventID, PInputEventID::KeyUp);
    EXPECT_EQ(m_Received.back().Event.KeySymbol, PKeySymbol(U'a'));
}

TEST_F(PKeyboardManagerTest, KeypadKeepsPhysicalIdentityAcrossNumLock)
{
    Send(PKeyCodes::KEYPAD_4, true);
    ASSERT_FALSE(m_Received.empty());
    EXPECT_EQ(m_Received.back().Event.KeyCode, PKeyCodes::KEYPAD_4);
    EXPECT_EQ(m_Received.back().Event.KeySymbol, PKeySymbol::LEFT_ARROW);
    EXPECT_TRUE(m_Received.back().Text.empty());
    Send(PKeyCodes::KEYPAD_4, false);
    Send(PKeyCodes::NUM_LOCK, true);
    Send(PKeyCodes::NUM_LOCK, false);
    Send(PKeyCodes::KEYPAD_4, true);
    EXPECT_EQ(m_Received.back().Event.KeyCode, PKeyCodes::KEYPAD_4);
    EXPECT_EQ(m_Received.back().Event.KeySymbol, PKeySymbol(U'4'));
    EXPECT_EQ(m_Received.back().Text, "4");
}

TEST_F(PKeyboardManagerTest, RemappingChangesLogicalSymbolWithoutChangingPhysicalKey)
{
    m_Manager.SetLayout("us", "", "caps:escape", false);
    Send(PKeyCodes::CAPS_LOCK, true);
    ASSERT_FALSE(m_Received.empty());
    EXPECT_EQ(m_Received.back().Event.KeyCode, PKeyCodes::CAPS_LOCK);
    EXPECT_EQ(m_Received.back().Event.KeySymbol, PKeySymbol::ESCAPE);
    EXPECT_TRUE(m_Received.back().Text.empty());
}

TEST_F(PKeyboardManagerTest, MainAndKeypadEnterShareLogicalSymbol)
{
    Send(PKeyCodes::ENTER, true);
    ASSERT_FALSE(m_Received.empty());
    EXPECT_EQ(m_Received.back().Event.KeyCode, PKeyCodes::ENTER);
    EXPECT_EQ(m_Received.back().Event.KeySymbol, PKeySymbol::ENTER);
    Send(PKeyCodes::ENTER, false);
    Send(PKeyCodes::KEYPAD_ENTER, true);
    EXPECT_EQ(m_Received.back().Event.KeyCode, PKeyCodes::KEYPAD_ENTER);
    EXPECT_EQ(m_Received.back().Event.KeySymbol, PKeySymbol::ENTER);
    Send(PKeyCodes::KEYPAD_ENTER, false);
    Send(PKeyCodes::F12, true);
    EXPECT_EQ(m_Received.back().Event.KeySymbol, PKeySymbol::F12);
}

TEST(KeyboardProtocol, CatalogRoundTripPreservesStrings)
{
    using Packer = PArgumentPacker<std::vector<PKeyboardLayoutInfo>>;
    const std::vector<PKeyboardLayoutInfo> layouts{{"no", "", "Norwegian"}, {"us", "intl", "International"}};
    std::vector<uint8_t> buffer(Packer::GetSize(layouts));
    ASSERT_EQ(Packer::Write(layouts, buffer.data(), buffer.size()), buffer.size());
    std::vector<PKeyboardLayoutInfo> decoded;
    ASSERT_EQ(Packer::Read(buffer.data(), buffer.size(), &decoded), buffer.size());
    ASSERT_EQ(decoded.size(), 2U);
    EXPECT_EQ(decoded[0].Layout, "no");
    EXPECT_EQ(decoded[1].Variant, "intl");
    EXPECT_EQ(decoded[1].Description, "International");
    EXPECT_LT(Packer::Read(buffer.data(), buffer.size() - 1, &decoded), 0);
}

TEST(KeyboardProtocol, CatalogKeepsExistingWireFormat)
{
    using Packer = PArgumentPacker<std::vector<PKeyboardLayoutInfo>>;
    const std::vector<PKeyboardLayoutInfo> layouts{{"no", "", "Norwegian"}};
    alignas(uint32_t) const uint8_t expected[] =
    {
        1, 0, 0, 0,
        2, 0, 0, 0, 'n', 'o', 0, 0,
        0, 0, 0, 0,
        9, 0, 0, 0, 'N', 'o', 'r', 'w', 'e', 'g', 'i', 'a', 'n', 0, 0, 0
    };
    std::vector<uint8_t> buffer(Packer::GetSize(layouts));
    ASSERT_EQ(buffer.size(), sizeof(expected));
    ASSERT_EQ(Packer::Write(layouts, buffer.data(), buffer.size()), buffer.size());
    EXPECT_EQ(std::memcmp(buffer.data(), expected, sizeof(expected)), 0);
    std::vector<PKeyboardLayoutInfo> decoded;
    ASSERT_EQ(Packer::Read(expected, sizeof(expected), &decoded), sizeof(expected));
    ASSERT_EQ(decoded.size(), 1U);
    EXPECT_EQ(decoded[0].Layout, "no");
    EXPECT_TRUE(decoded[0].Variant.empty());
    EXPECT_EQ(decoded[0].Description, "Norwegian");
}
