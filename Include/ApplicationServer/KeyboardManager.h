// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <array>
#include <bitset>
#include <map>
#include <memory>
#include <set>
#include <vector>

#include <GUI/GUIEvent.h>
#include <GUI/KeyboardLayout.h>
#include <Ptr/Ptr.h>
#include <Signals/Signal.h>
#include <Threads/EventTimer.h>
#include <xkbcommon/xkbcommon.h>
#include <xkbcommon/xkbcommon-compose.h>

class PKeyboardManager : public SignalTarget
{
public:
    explicit PKeyboardManager(PLooper& looper);
    // Reuse a configured manager's immutable data with fresh input state.
    // Managers sharing XKB data must be used and destroyed on the same thread.
    PKeyboardManager(PLooper& looper, const PKeyboardManager& configuration);
    ~PKeyboardManager();

    void Initialize();
    const std::vector<PKeyboardLayoutInfo>& GetLayouts();
    const PString&  GetLayout() const { return m_Layout; }
    const PString&  GetVariant() const { return m_Variant; }
    const PString&  GetOptions() const { return m_Options; }
    bool            IsConfigured() const { return m_State != nullptr; }
    uint32_t        GetQualifiers() const { return m_Qualifiers; }
    void            SetLayout(const PString& layout, const PString& variant, const PString& options, bool persist = true);

    void AddDevice(int32_t sourceID);
    void RemoveDevice(int32_t sourceID);
    void HandleKeyEvent(const PRawKeyEvent& event);
    void SetFocused(bool focused);

    Signal<void, const PKeyEvent&, const PString&> SignalKeyEvent;

private:
    using Keymap            = std::unique_ptr<xkb_keymap, decltype(&xkb_keymap_unref)>;
    using State             = std::unique_ptr<xkb_state, decltype(&xkb_state_unref)>;
    using PhysicalKeyMap    = std::array<xkb_keycode_t, KEY_CODE_LIMIT>;

    struct LayoutCatalog : public PtrTarget
    {
        std::vector<PKeyboardLayoutInfo> Layouts;
        std::set<PString> Options;
    };

    struct DeviceState
    {
        std::array<PKeySymbol, KEY_CODE_LIMIT> KeySymbols{};
        std::bitset<KEY_CODE_LIMIT> Pressed;
    };

    void                    LoadCatalog();
    void                    LoadConfiguration();
    void                    SaveConfiguration(const PString& layout, const PString& variant, const PString& options);
    static PhysicalKeyMap   BuildPhysicalKeyMap(xkb_keymap* keymap);
    void                    RestoreState(xkb_state* state, const PhysicalKeyMap& physicalKeyMap) const;
    PString                 TranslateText(xkb_keycode_t keycode, xkb_keysym_t keysym, bool feedCompose, bool& composedResult);
    uint32_t                CalculateQualifiers() const;
    static PKeySymbol       GetKeySymbol(xkb_keysym_t keysym);
    PKeyEvent               MakeKeyEvent(const PRawKeyEvent& event, PKeySymbol keySymbol) const;
    void                    SlotRepeat(PEventTimer* timer);


    PLooper& m_Looper;
    std::unique_ptr<xkb_context, decltype(&xkb_context_unref)> m_Context{nullptr, xkb_context_unref};
    
    Keymap  m_Keymap{nullptr, xkb_keymap_unref};
    State   m_State{nullptr, xkb_state_unref};

    std::unique_ptr<xkb_compose_table, decltype(&xkb_compose_table_unref)> m_ComposeTable{nullptr, xkb_compose_table_unref};
    std::unique_ptr<xkb_compose_state, decltype(&xkb_compose_state_unref)> m_ComposeState{nullptr, xkb_compose_state_unref};

    PhysicalKeyMap m_PhysicalKeyMap{};

    std::array<uint32_t, KEY_CODE_LIMIT>    m_PressedCounts{};
    std::map<int32_t, DeviceState>          m_Devices;
    Ptr<const LayoutCatalog>                m_Catalog;

    PString         m_Layout;
    PString         m_Variant;
    PString         m_Options;
    PEventTimer     m_RepeatTimer;
    PRawKeyEvent    m_RepeatEvent{};
    PString         m_RepeatText;
    uint32_t        m_Qualifiers = 0;
    bool            m_RepeatComposed = false;
    bool            m_Focused = false;
};
