// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <ApplicationServer/KeyboardManager.h>

#include <algorithm>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <pugixml.hpp>
#include <xkbcommon/xkbcommon-keysyms.h>
#include <xkbcommon/xkbcommon-names.h>

#include <ApplicationServer/ApplicationServer.h>
#include <PadOS/Time.h>
#include <Storage/Path.h>
#include <Storage/StandardPaths.h>
#include <System/ExceptionHandling.h>

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PKeyboardManager::PKeyboardManager(PLooper& looper)
    : m_Looper(looper)
{
    m_Context.reset(xkb_context_new(xkb_context_flags(XKB_CONTEXT_NO_DEFAULT_INCLUDES | XKB_CONTEXT_NO_ENVIRONMENT_NAMES)));
    if (m_Context == nullptr) {
        PERROR_THROW_CODE(PErrorCode::NOMEM);
    }
    m_RepeatTimer.SignalTrigged.Connect(this, &PKeyboardManager::SlotRepeat);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PKeyboardManager::~PKeyboardManager()
{
    m_RepeatTimer.Stop();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PKeyboardManager::Initialize()
{
    LoadCatalog();
    LoadConfiguration();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

const std::vector<PKeyboardLayoutInfo>& PKeyboardManager::GetLayouts()
{
    if (m_Layouts.empty()) {
        LoadCatalog();
    }
    return m_Layouts;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PKeyboardManager::SetLayout(const PString& layout, const PString& variant, const PString& options, bool persist)
{
    const std::vector<PKeyboardLayoutInfo>& layouts = GetLayouts();
    const auto selected = std::find_if(layouts.begin(), layouts.end(), [&layout, &variant](const PKeyboardLayoutInfo& entry)
    {
        return entry.Layout == layout && entry.Variant == variant;
    });
    if (selected == layouts.end()) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    for (size_t offset = 0; offset < options.size();)
    {
        const size_t end = options.find(',', offset);
        const PString option = options.substr(offset, (end == PString::npos) ? end : end - offset);
        if (!m_KnownOptions.contains(option)) {
            PERROR_THROW_CODE(PErrorCode::INVAL);
        }
        offset = (end == PString::npos) ? options.size() : end + 1;
    }

    PString newLayout = layout;
    PString newVariant = variant;
    PString newOptions = options;
    xkb_context_include_path_clear(m_Context.get());
    if (access("/etc/xkb", R_OK) == 0) {
        xkb_context_include_path_append(m_Context.get(), "/etc/xkb");
    }
    if (xkb_context_include_path_append(m_Context.get(), "/usr/share/X11/xkb") == 0) {
        PERROR_THROW_CODE(PErrorCode::NOENT);
    }
    const xkb_rule_names names{"evdev", "pc105", layout.c_str(), variant.c_str(), options.c_str()};
    Keymap keymap(
        xkb_keymap_new_from_names2(m_Context.get(), &names, XKB_KEYMAP_FORMAT_TEXT_V2, XKB_KEYMAP_COMPILE_NO_FLAGS),
        xkb_keymap_unref);
    if (keymap == nullptr) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    State state(xkb_state_new(keymap.get()), xkb_state_unref);
    if (state == nullptr) {
        PERROR_THROW_CODE(PErrorCode::NOMEM);
    }
    if (m_ComposeTable == nullptr)
    {
        m_ComposeTable.reset(xkb_compose_table_new_from_locale(m_Context.get(), "en_US.UTF-8", XKB_COMPOSE_COMPILE_NO_FLAGS));
        if (m_ComposeTable == nullptr) {
            PERROR_THROW_CODE(PErrorCode::NOENT);
        }
    }
    if (m_ComposeState == nullptr)
    {
        m_ComposeState.reset(xkb_compose_state_new(m_ComposeTable.get(), XKB_COMPOSE_STATE_NO_FLAGS));
        if (m_ComposeState == nullptr) {
            PERROR_THROW_CODE(PErrorCode::NOMEM);
        }
    }
    const PhysicalKeyMap physicalKeyMap = BuildPhysicalKeyMap(keymap.get());
    RestoreState(state.get(), physicalKeyMap);
    if (persist) {
        SaveConfiguration(layout, variant, options);
    }

    m_RepeatTimer.Stop();
    xkb_compose_state_reset(m_ComposeState.get());
    m_State = std::move(state);
    m_Keymap = std::move(keymap);
    m_PhysicalKeyMap = physicalKeyMap;
    m_Layout.swap(newLayout);
    m_Variant.swap(newVariant);
    m_Options.swap(newOptions);
    m_Qualifiers = CalculateQualifiers();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PKeyboardManager::AddDevice(int32_t sourceID)
{
    m_Devices.try_emplace(sourceID);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PKeyboardManager::RemoveDevice(int32_t sourceID)
{
    const auto device = m_Devices.find(sourceID);
    if (device == m_Devices.end()) {
        return;
    }
    for (size_t keyIndex = 0; keyIndex < KEY_CODE_LIMIT; ++keyIndex)
    {
        if (device->second.Pressed[keyIndex])
        {
            PRawKeyEvent event{};
            event.EventSize = sizeof(event);
            event.EventType = PInputEventType::RawKeyEvent;
            event.ClassID = PInputClass::Keyboard;
            event.Timestamp = get_monotonic_time();
            event.EventID = PInputEventID::KeyUp;
            event.SourceID = sourceID;
            event.KeyCode = static_cast<PKeyCodes>(keyIndex);
            HandleKeyEvent(event);
        }
    }
    m_Devices.erase(device);
    if (m_ComposeState != nullptr) {
        xkb_compose_state_reset(m_ComposeState.get());
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PKeyboardManager::HandleKeyEvent(const PRawKeyEvent& event)
{
    const size_t keyIndex = std::to_underlying(event.KeyCode);
    if (event.KeyCode == PKeyCodes::NONE || keyIndex >= KEY_CODE_LIMIT) {
        return;
    }
    const auto device = m_Devices.find(event.SourceID);
    if (device == m_Devices.end()) {
        return;
    }
    const bool pressed = event.EventID == PInputEventID::KeyDown;
    if (device->second.Pressed[keyIndex] == pressed) {
        return;
    }
    const xkb_keycode_t keycode = m_PhysicalKeyMap[keyIndex];
    const bool mapped = m_State != nullptr && keycode != XKB_KEYCODE_INVALID;
    const xkb_keysym_t keysym = (pressed && mapped) ? xkb_state_key_get_one_sym(m_State.get(), keycode) : XKB_KEY_NoSymbol;
    const PKeySymbol keySymbol = pressed ? GetKeySymbol(keysym) : device->second.KeySymbols[keyIndex];
    PString text;
    bool composed = false;
    if (pressed && mapped && m_Focused) {
        text = TranslateText(keycode, keysym, true, composed);
    }
    if (pressed)
    {
        device->second.Pressed.set(keyIndex);
        device->second.KeySymbols[keyIndex] = keySymbol;
        if (m_PressedCounts[keyIndex]++ == 0 && mapped) {
            xkb_state_update_key(m_State.get(), keycode, XKB_KEY_DOWN);
        }
    }
    else
    {
        device->second.Pressed.reset(keyIndex);
        if (--m_PressedCounts[keyIndex] == 0 && mapped) {
            xkb_state_update_key(m_State.get(), keycode, XKB_KEY_UP);
        }
        if (event.SourceID == m_RepeatEvent.SourceID && event.KeyCode == m_RepeatEvent.KeyCode) {
            m_RepeatTimer.Stop();
        }
    }
    m_Qualifiers = CalculateQualifiers();
    const PKeyEvent translated = MakeKeyEvent(event, keySymbol);
    if (m_Focused) {
        SignalKeyEvent(translated, text);
    }
    if (m_ComposeState != nullptr && xkb_compose_state_get_status(m_ComposeState.get()) == XKB_COMPOSE_COMPOSING) {
        m_RepeatTimer.Stop();
    }
    if (pressed && mapped && m_Focused && xkb_keymap_key_repeats(m_Keymap.get(), keycode)
        && xkb_compose_state_get_status(m_ComposeState.get()) != XKB_COMPOSE_COMPOSING)
    {
        m_RepeatEvent = event;
        m_RepeatText = text;
        m_RepeatComposed = composed;
        m_RepeatTimer.Set(TimeValNanos::FromSeconds(KEYREPEAT_DELAY), true);
        m_RepeatTimer.Start(true, &m_Looper);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PKeyboardManager::SetFocused(bool focused)
{
    m_Focused = focused;
    m_RepeatTimer.Stop();
    if (m_ComposeState != nullptr) {
        xkb_compose_state_reset(m_ComposeState.get());
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PKeyboardManager::LoadCatalog()
{
    std::vector<PKeyboardLayoutInfo> layouts;
    std::set<PString> options;
    for (const char* filename : {"/usr/share/X11/xkb/rules/evdev.xml", "/usr/share/X11/xkb/rules/evdev.extras.xml"})
    {
        pugi::xml_document document;
        const pugi::xml_parse_result result = document.load_file(filename);
        if (!result) {
            PERROR_THROW_CODE((result.status == pugi::status_file_not_found) ? PErrorCode::NOENT : PErrorCode::INVAL);
        }
        const pugi::xml_node registry = document.child("xkbConfigRegistry");
        for (const pugi::xml_node layout : registry.child("layoutList").children("layout"))
        {
            const pugi::xml_node item = layout.child("configItem");
            const PString name = item.child_value("name");
            // The upstream custom entry is a placeholder until its symbols file exists.
            if (access((PString("/usr/share/X11/xkb/symbols/") + name).c_str(), R_OK) != 0) {
                continue;
            }
            layouts.push_back({name, PString(), PString(item.child_value("description"))});
            for (const pugi::xml_node variant : layout.child("variantList").children("variant"))
            {
                const pugi::xml_node variantItem = variant.child("configItem");
                layouts.push_back(
                    {name, PString(variantItem.child_value("name")), PString(variantItem.child_value("description"))});
            }
        }
        for (const pugi::xml_node group : registry.child("optionList").children("group")) {
            for (const pugi::xml_node option : group.children("option")) {
                options.insert(PString(option.child("configItem").child_value("name")));
            }
        }
    }
    if (layouts.empty()) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    // Extras can extend a layout already present in the main catalog.
    std::stable_sort(layouts.begin(), layouts.end(), [](const PKeyboardLayoutInfo& lhs, const PKeyboardLayoutInfo& rhs)
    {
        return (lhs.Layout == rhs.Layout) ? lhs.Variant < rhs.Variant : lhs.Layout < rhs.Layout;
    });
    const auto end = std::unique(
        layouts.begin(),
        layouts.end(),
        [](const PKeyboardLayoutInfo& lhs, const PKeyboardLayoutInfo& rhs)
        {
            return lhs.Layout == rhs.Layout && lhs.Variant == rhs.Variant;
        });
    layouts.erase(end, layouts.end());
    m_Layouts.swap(layouts);
    m_KnownOptions.swap(options);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PKeyboardManager::LoadConfiguration()
{
    PString layout = "us";
    PString variant;
    PString options;
    const PString directory = PStandardPaths::GetPath(PStandardPath::Settings);
    pugi::xml_document document;
    if (!directory.empty() && document.load_file((directory + "/Keyboard.xml").c_str()))
    {
        const pugi::xml_node configuration = document.child("Keyboard");
        layout = configuration.attribute("layout").as_string("us");
        variant = configuration.attribute("variant").as_string();
        options = configuration.attribute("options").as_string();
    }
    try {
        SetLayout(layout, variant, options, false);
    }
    PERROR_CATCH([this](PErrorCode error)
    {
        p_system_log<PLogSeverity::WARNING>(
            LogCategoryAppServer,
            "Cannot load saved keyboard configuration: {}",
            std::to_underlying(error));
        SetLayout("us", "", "", false);
    });
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PKeyboardManager::SaveConfiguration(const PString& layout, const PString& variant, const PString& options)
{
    const PString directory = PStandardPaths::GetPath(PStandardPath::Settings);
    if (directory.empty()) {
        PERROR_THROW_CODE(PErrorCode::NOENT);
    }
    const PString filename = directory + "/Keyboard.xml";
    const PString temporary = filename + ".tmp";
    PPath(filename).CreateFolders(false);
    pugi::xml_document document;
    pugi::xml_node configuration = document.append_child("Keyboard");
    configuration.append_attribute("layout") = layout.c_str();
    configuration.append_attribute("variant") = variant.c_str();
    configuration.append_attribute("options") = options.c_str();
    PScopeExit cleanup([&temporary]() { unlink(temporary.c_str()); });
    if (!document.save_file(temporary.c_str())) {
        PERROR_THROW_CODE(PErrorCode::IO);
    }
    if (rename(temporary.c_str(), filename.c_str()) != 0) {
        PERROR_THROW_CODE(PErrorCode(errno));
    }
    cleanup.release();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PKeyboardManager::PhysicalKeyMap PKeyboardManager::BuildPhysicalKeyMap(xkb_keymap* keymap)
{
    // PadOS physical key codes to XKB key names. Resolve once per keymap;
    // event handling uses direct array lookups and does not depend on evdev numbers.
    struct PhysicalKeyName
    {
        PKeyCodes KeyCode;
        const char* Name;
    };
    static constexpr PhysicalKeyName PHYSICAL_KEY_NAMES[] =
    {
        {PKeyCodes::A, "AC01"}, {PKeyCodes::B, "AB05"}, {PKeyCodes::C, "AB03"}, {PKeyCodes::D, "AC03"}, {PKeyCodes::E, "AD03"},
        {PKeyCodes::F, "AC04"}, {PKeyCodes::G, "AC05"}, {PKeyCodes::H, "AC06"}, {PKeyCodes::I, "AD08"}, {PKeyCodes::J, "AC07"},
        {PKeyCodes::K, "AC08"}, {PKeyCodes::L, "AC09"}, {PKeyCodes::M, "AB07"}, {PKeyCodes::N, "AB06"}, {PKeyCodes::O, "AD09"},
        {PKeyCodes::P, "AD10"}, {PKeyCodes::Q, "AD01"}, {PKeyCodes::R, "AD04"}, {PKeyCodes::S, "AC02"}, {PKeyCodes::T, "AD05"},
        {PKeyCodes::U, "AD07"}, {PKeyCodes::V, "AB04"}, {PKeyCodes::W, "AD02"}, {PKeyCodes::X, "AB02"}, {PKeyCodes::Y, "AD06"},
        {PKeyCodes::Z, "AB01"}, {PKeyCodes::NUM_1, "AE01"}, {PKeyCodes::NUM_2, "AE02"}, {PKeyCodes::NUM_3, "AE03"},
        {PKeyCodes::NUM_4, "AE04"}, {PKeyCodes::NUM_5, "AE05"}, {PKeyCodes::NUM_6, "AE06"}, {PKeyCodes::NUM_7, "AE07"},
        {PKeyCodes::NUM_8, "AE08"}, {PKeyCodes::NUM_9, "AE09"}, {PKeyCodes::NUM_0, "AE10"}, {PKeyCodes::ENTER, "RTRN"},
        {PKeyCodes::ESCAPE, "ESC"}, {PKeyCodes::BACKSPACE, "BKSP"}, {PKeyCodes::TAB, "TAB"}, {PKeyCodes::SPACE, "SPCE"},
        {PKeyCodes::MINUS, "AE11"}, {PKeyCodes::EQUAL, "AE12"}, {PKeyCodes::LEFT_BRACKET, "AD11"},
        {PKeyCodes::RIGHT_BRACKET, "AD12"}, {PKeyCodes::BACKSLASH, "BKSL"}, {PKeyCodes::NON_US_HASH, "BKSL"},
        {PKeyCodes::SEMICOLON, "AC10"}, {PKeyCodes::APOSTROPHE, "AC11"}, {PKeyCodes::GRAVE, "TLDE"}, {PKeyCodes::COMMA, "AB08"},
        {PKeyCodes::PERIOD, "AB09"}, {PKeyCodes::SLASH, "AB10"}, {PKeyCodes::CAPS_LOCK, "CAPS"}, {PKeyCodes::F1, "FK01"},
        {PKeyCodes::F2, "FK02"}, {PKeyCodes::F3, "FK03"}, {PKeyCodes::F4, "FK04"}, {PKeyCodes::F5, "FK05"},
        {PKeyCodes::F6, "FK06"}, {PKeyCodes::F7, "FK07"}, {PKeyCodes::F8, "FK08"}, {PKeyCodes::F9, "FK09"},
        {PKeyCodes::F10, "FK10"}, {PKeyCodes::F11, "FK11"}, {PKeyCodes::F12, "FK12"}, {PKeyCodes::PRINT_SCREEN, "PRSC"},
        {PKeyCodes::SCROLL_LOCK, "SCLK"}, {PKeyCodes::PAUSE, "PAUS"}, {PKeyCodes::INSERT, "INS"}, {PKeyCodes::HOME, "HOME"},
        {PKeyCodes::PAGE_UP, "PGUP"}, {PKeyCodes::DELETE, "DELE"}, {PKeyCodes::END, "END"}, {PKeyCodes::PAGE_DOWN, "PGDN"},
        {PKeyCodes::RIGHT_ARROW, "RGHT"}, {PKeyCodes::LEFT_ARROW, "LEFT"}, {PKeyCodes::DOWN_ARROW, "DOWN"},
        {PKeyCodes::UP_ARROW, "UP"}, {PKeyCodes::NUM_LOCK, "NMLK"}, {PKeyCodes::KEYPAD_DIVIDE, "KPDV"},
        {PKeyCodes::KEYPAD_MULTIPLY, "KPMU"}, {PKeyCodes::KEYPAD_SUBTRACT, "KPSU"}, {PKeyCodes::KEYPAD_ADD, "KPAD"},
        {PKeyCodes::KEYPAD_ENTER, "KPEN"}, {PKeyCodes::KEYPAD_1, "KP1"}, {PKeyCodes::KEYPAD_2, "KP2"},
        {PKeyCodes::KEYPAD_3, "KP3"}, {PKeyCodes::KEYPAD_4, "KP4"}, {PKeyCodes::KEYPAD_5, "KP5"}, {PKeyCodes::KEYPAD_6, "KP6"},
        {PKeyCodes::KEYPAD_7, "KP7"}, {PKeyCodes::KEYPAD_8, "KP8"}, {PKeyCodes::KEYPAD_9, "KP9"}, {PKeyCodes::KEYPAD_0, "KP0"},
        {PKeyCodes::KEYPAD_PERIOD, "KPDL"}, {PKeyCodes::NON_US_BACKSLASH, "LSGT"}, {PKeyCodes::APPLICATION, "COMP"},
        {PKeyCodes::POWER, "POWR"}, {PKeyCodes::KEYPAD_EQUAL, "KPEQ"}, {PKeyCodes::F13, "FK13"}, {PKeyCodes::F14, "FK14"},
        {PKeyCodes::F15, "FK15"}, {PKeyCodes::F16, "FK16"}, {PKeyCodes::F17, "FK17"}, {PKeyCodes::F18, "FK18"},
        {PKeyCodes::F19, "FK19"}, {PKeyCodes::F20, "FK20"}, {PKeyCodes::F21, "FK21"}, {PKeyCodes::F22, "FK22"},
        {PKeyCodes::F23, "FK23"}, {PKeyCodes::F24, "FK24"}, {PKeyCodes::HELP, "HELP"}, {PKeyCodes::MENU, "MENU"},
        {PKeyCodes::STOP, "STOP"}, {PKeyCodes::AGAIN, "AGAI"}, {PKeyCodes::UNDO, "UNDO"}, {PKeyCodes::CUT, "CUT"},
        {PKeyCodes::COPY, "COPY"}, {PKeyCodes::PASTE, "PAST"}, {PKeyCodes::FIND, "FIND"}, {PKeyCodes::MUTE, "MUTE"},
        {PKeyCodes::VOLUME_UP, "VOL+"}, {PKeyCodes::VOLUME_DOWN, "VOL-"}, {PKeyCodes::KEYPAD_COMMA, "KPPT"},
        {PKeyCodes::INTERNATIONAL_1, "AB11"}, {PKeyCodes::INTERNATIONAL_2, "HKTG"}, {PKeyCodes::INTERNATIONAL_3, "AE13"},
        {PKeyCodes::INTERNATIONAL_4, "HENK"}, {PKeyCodes::INTERNATIONAL_5, "MUHE"}, {PKeyCodes::INTERNATIONAL_6, "JPCM"},
        {PKeyCodes::LANGUAGE_1, "HNGL"}, {PKeyCodes::LANGUAGE_2, "HJCV"}, {PKeyCodes::LANGUAGE_3, "KATA"},
        {PKeyCodes::LANGUAGE_4, "HIRA"}, {PKeyCodes::LANGUAGE_5, "HZTG"}, {PKeyCodes::LEFT_CONTROL, "LCTL"},
        {PKeyCodes::LEFT_SHIFT, "LFSH"}, {PKeyCodes::LEFT_ALT, "LALT"}, {PKeyCodes::LEFT_GUI, "LWIN"},
        {PKeyCodes::RIGHT_CONTROL, "RCTL"}, {PKeyCodes::RIGHT_SHIFT, "RTSH"}, {PKeyCodes::RIGHT_ALT, "RALT"},
        {PKeyCodes::RIGHT_GUI, "RWIN"}
    };
    PhysicalKeyMap result;
    result.fill(XKB_KEYCODE_INVALID);
    for (const PhysicalKeyName& entry : PHYSICAL_KEY_NAMES) {
        result[std::to_underlying(entry.KeyCode)] = xkb_keymap_key_by_name(keymap, entry.Name);
    }
    return result;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PKeyboardManager::RestoreState(xkb_state* state, const PhysicalKeyMap& physicalKeyMap) const
{
    for (size_t keyIndex = KEY_FIRST_MODIFIER_INDEX; keyIndex <= KEY_LAST_MODIFIER_INDEX; ++keyIndex) {
        if (m_PressedCounts[keyIndex] != 0 && physicalKeyMap[keyIndex] != XKB_KEYCODE_INVALID) {
            xkb_state_update_key(state, physicalKeyMap[keyIndex], XKB_KEY_DOWN);
        }
    }
    for (size_t keyIndex = 0; keyIndex < KEY_FIRST_MODIFIER_INDEX; ++keyIndex) {
        if (m_PressedCounts[keyIndex] != 0 && physicalKeyMap[keyIndex] != XKB_KEYCODE_INVALID) {
            xkb_state_update_key(state, physicalKeyMap[keyIndex], XKB_KEY_DOWN);
        }
    }
    if (m_State != nullptr)
    {
        xkb_mod_mask_t locked = 0;
        xkb_keymap* keymap = xkb_state_get_keymap(state);
        const xkb_mod_index_t count = xkb_keymap_num_mods(keymap);
        for (xkb_mod_index_t index = 0; index < count; ++index)
        {
            const char* name = xkb_keymap_mod_get_name(keymap, index);
            if (xkb_state_mod_name_is_active(m_State.get(), name, XKB_STATE_MODS_LOCKED) > 0) {
                locked |= xkb_mod_mask_t(1) << index;
            }
        }
        xkb_state_update_latched_locked(state, 0, 0, false, 0, ~xkb_mod_mask_t(0), locked, false, 0);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PString PKeyboardManager::TranslateText(xkb_keycode_t keycode, xkb_keysym_t keysym, bool feedCompose, bool& composedResult)
{
    composedResult = false;
    if ((GetQualifiers() & (QUAL_CTRL | QUAL_SUPER | QUAL_ALT)) != 0) {
        return PString();
    }
    if (feedCompose) {
        xkb_compose_state_feed(m_ComposeState.get(), keysym);
    }
    const xkb_compose_status status = xkb_compose_state_get_status(m_ComposeState.get());
    if (status == XKB_COMPOSE_COMPOSING) {
        return PString();
    }
    if (status == XKB_COMPOSE_CANCELLED)
    {
        xkb_compose_state_reset(m_ComposeState.get());
        return PString();
    }
    const bool composed = status == XKB_COMPOSE_COMPOSED;
    const int length = composed
        ? xkb_compose_state_get_utf8(m_ComposeState.get(), nullptr, 0)
        : xkb_state_key_get_utf8(m_State.get(), keycode, nullptr, 0);
    PString text;
    if (length > 0)
    {
        text.resize(static_cast<size_t>(length));
        if (composed) {
            xkb_compose_state_get_utf8(m_ComposeState.get(), text.data(), text.size() + 1);
        } else {
            xkb_state_key_get_utf8(m_State.get(), keycode, text.data(), text.size() + 1);
        }
        if (text == "\r") {
            text = "\n";
        } else if (static_cast<uint8_t>(text[0]) < 0x20 && text != "\n" && text != "\t") {
            text.clear();
        } else if (text == "\x7f") {
            text.clear();
        }
    }
    if (composed)
    {
        xkb_compose_state_reset(m_ComposeState.get());
        composedResult = true;
    }
    return text;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

uint32_t PKeyboardManager::CalculateQualifiers() const
{
    if (m_State == nullptr) {
        return 0;
    }
    uint32_t result = 0;
    if (xkb_state_mod_name_is_active(m_State.get(), XKB_MOD_NAME_SHIFT, XKB_STATE_MODS_EFFECTIVE) > 0)
    {
        result |= (m_PressedCounts[std::to_underlying(PKeyCodes::RIGHT_SHIFT)] != 0) ? QUAL_RSHIFT : QUAL_LSHIFT;
        if (m_PressedCounts[std::to_underlying(PKeyCodes::LEFT_SHIFT)] != 0) {
            result |= QUAL_LSHIFT;
        }
    }
    if (xkb_state_mod_name_is_active(m_State.get(), XKB_MOD_NAME_CTRL, XKB_STATE_MODS_EFFECTIVE) > 0)
    {
        result |= (m_PressedCounts[std::to_underlying(PKeyCodes::RIGHT_CONTROL)] != 0) ? QUAL_RCTRL : QUAL_LCTRL;
        if (m_PressedCounts[std::to_underlying(PKeyCodes::LEFT_CONTROL)] != 0) {
            result |= QUAL_LCTRL;
        }
    }
    if (xkb_state_mod_name_is_active(m_State.get(), XKB_MOD_NAME_ALT, XKB_STATE_MODS_EFFECTIVE) > 0)
    {
        result |= (m_PressedCounts[std::to_underlying(PKeyCodes::RIGHT_ALT)] != 0) ? QUAL_RALT : QUAL_LALT;
        if (m_PressedCounts[std::to_underlying(PKeyCodes::LEFT_ALT)] != 0) {
            result |= QUAL_LALT;
        }
    }
    if (xkb_state_mod_name_is_active(m_State.get(), XKB_MOD_NAME_LOGO, XKB_STATE_MODS_EFFECTIVE) > 0) {
        result |= QUAL_SUPER;
    }
    if (xkb_state_mod_name_is_active(m_State.get(), XKB_MOD_NAME_CAPS, XKB_STATE_MODS_LOCKED) > 0) {
        result |= QUAL_CAPS_LOCK;
    }
    if (xkb_state_mod_name_is_active(m_State.get(), XKB_MOD_NAME_NUM, XKB_STATE_MODS_LOCKED) > 0) {
        result |= QUAL_NUM_LOCK;
    }
    if (xkb_state_mod_name_is_active(m_State.get(), "Mod5", XKB_STATE_MODS_EFFECTIVE) > 0) {
        result |= QUAL_ALT_GR;
    }
    return result;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PKeySymbol PKeyboardManager::GetKeySymbol(xkb_keysym_t keysym)
{
    // Latin-1 keysyms use their Unicode values directly.
    uint32_t character = keysym;
    if (keysym >= 0x100)
    {
        if (keysym >= XKB_KEY_F1 && keysym <= XKB_KEY_F35) {
            return PKeySymbol(std::to_underlying(PKeySymbol::F1) + keysym - XKB_KEY_F1);
        }
        if (keysym >= XKB_KEY_KP_F1 && keysym <= XKB_KEY_KP_F4) {
            return PKeySymbol(std::to_underlying(PKeySymbol::F1) + keysym - XKB_KEY_KP_F1);
        }
        switch (keysym)
        {
            case XKB_KEY_Left:
            case XKB_KEY_KP_Left:
                return PKeySymbol::LEFT_ARROW;
            case XKB_KEY_Right:
            case XKB_KEY_KP_Right:
                return PKeySymbol::RIGHT_ARROW;
            case XKB_KEY_Up:
            case XKB_KEY_KP_Up:
                return PKeySymbol::UP_ARROW;
            case XKB_KEY_Down:
            case XKB_KEY_KP_Down:
                return PKeySymbol::DOWN_ARROW;
            case XKB_KEY_Home:
            case XKB_KEY_KP_Home:
                return PKeySymbol::HOME;
            case XKB_KEY_End:
            case XKB_KEY_KP_End:
                return PKeySymbol::END;
            case XKB_KEY_Delete:
            case XKB_KEY_KP_Delete:
                return PKeySymbol::DELETE;
            case XKB_KEY_Insert:
            case XKB_KEY_KP_Insert:
                return PKeySymbol::INSERT;
            case XKB_KEY_Prior:
            case XKB_KEY_KP_Prior:
                return PKeySymbol::PAGE_UP;
            case XKB_KEY_Next:
            case XKB_KEY_KP_Next:
                return PKeySymbol::PAGE_DOWN;
            case XKB_KEY_Begin:
            case XKB_KEY_KP_Begin:
                return PKeySymbol::BEGIN;
            case XKB_KEY_BackSpace:
                return PKeySymbol::BACKSPACE;
            case XKB_KEY_Tab:
            case XKB_KEY_ISO_Left_Tab:
            case XKB_KEY_KP_Tab:
                return PKeySymbol::TAB;
            case XKB_KEY_Return:
            case XKB_KEY_KP_Enter:
                return PKeySymbol::ENTER;
            case XKB_KEY_Escape:
                return PKeySymbol::ESCAPE;
            case XKB_KEY_Shift_L:
                return PKeySymbol::LEFT_SHIFT;
            case XKB_KEY_Shift_R:
                return PKeySymbol::RIGHT_SHIFT;
            case XKB_KEY_Control_L:
                return PKeySymbol::LEFT_CONTROL;
            case XKB_KEY_Control_R:
                return PKeySymbol::RIGHT_CONTROL;
            case XKB_KEY_Alt_L:
                return PKeySymbol::LEFT_ALT;
            case XKB_KEY_Alt_R:
                return PKeySymbol::RIGHT_ALT;
            case XKB_KEY_ISO_Level3_Shift:
                return PKeySymbol::ALT_GR;
            case XKB_KEY_ISO_Level5_Shift:
                return PKeySymbol::LEVEL5_SHIFT;
            case XKB_KEY_Super_L:
                return PKeySymbol::LEFT_SUPER;
            case XKB_KEY_Super_R:
                return PKeySymbol::RIGHT_SUPER;
            case XKB_KEY_Meta_L:
                return PKeySymbol::LEFT_META;
            case XKB_KEY_Meta_R:
                return PKeySymbol::RIGHT_META;
            case XKB_KEY_Hyper_L:
                return PKeySymbol::LEFT_HYPER;
            case XKB_KEY_Hyper_R:
                return PKeySymbol::RIGHT_HYPER;
            case XKB_KEY_Caps_Lock:
                return PKeySymbol::CAPS_LOCK;
            case XKB_KEY_Shift_Lock:
                return PKeySymbol::SHIFT_LOCK;
            case XKB_KEY_Num_Lock:
                return PKeySymbol::NUM_LOCK;
            case XKB_KEY_Scroll_Lock:
                return PKeySymbol::SCROLL_LOCK;
            case XKB_KEY_Print:
                return PKeySymbol::PRINT_SCREEN;
            case XKB_KEY_Sys_Req:
                return PKeySymbol::SYSTEM_REQUEST;
            case XKB_KEY_Pause:
                return PKeySymbol::PAUSE;
            case XKB_KEY_Break:
                return PKeySymbol::BREAK;
            case XKB_KEY_Menu:
                return PKeySymbol::MENU;
            case XKB_KEY_Clear:
                return PKeySymbol::CLEAR;
            case XKB_KEY_Select:
                return PKeySymbol::SELECT;
            case XKB_KEY_Execute:
                return PKeySymbol::EXECUTE;
            case XKB_KEY_Cancel:
                return PKeySymbol::CANCEL;
            case XKB_KEY_Help:
                return PKeySymbol::HELP;
            case XKB_KEY_Undo:
                return PKeySymbol::UNDO;
            case XKB_KEY_Redo:
                return PKeySymbol::REDO;
            case XKB_KEY_Find:
                return PKeySymbol::FIND;
            case XKB_KEY_XF86Cut:
                return PKeySymbol::CUT;
            case XKB_KEY_XF86Copy:
                return PKeySymbol::COPY;
            case XKB_KEY_XF86Paste:
                return PKeySymbol::PASTE;
            case XKB_KEY_XF86PowerOff:
                return PKeySymbol::POWER;
            case XKB_KEY_XF86AudioMute:
                return PKeySymbol::MUTE;
            case XKB_KEY_XF86AudioRaiseVolume:
                return PKeySymbol::VOLUME_UP;
            case XKB_KEY_XF86AudioLowerVolume:
                return PKeySymbol::VOLUME_DOWN;
            case XKB_KEY_Multi_key:
                return PKeySymbol::COMPOSE;
            default: break;
        }
        // Character conversion removes XKB's legacy and Unicode encodings from the public event.
        // Unrepresented non-character symbols (including dead keys) have no public symbol.
        // Their composition is still handled internally and delivered as UTF-8 text.
        character = xkb_keysym_to_utf32(keysym);
    }
    return ((character >= 0x20 && character < 0x7f) || character >= 0xa0) ? PKeySymbol(character) : PKeySymbol::NONE;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PKeyEvent PKeyboardManager::MakeKeyEvent(const PRawKeyEvent& event, PKeySymbol keySymbol) const
{
    PKeyEvent translated{};
    static_cast<PInputEvent&>(translated) = event;
    translated.EventSize = sizeof(translated);
    translated.EventType = PInputEventType::KeyEvent;
    translated.KeyCode = event.KeyCode;
    translated.KeySymbol = keySymbol;
    translated.Qualifiers = GetQualifiers();
    return translated;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PKeyboardManager::SlotRepeat(PEventTimer* timer)
{
    const size_t keyIndex = std::to_underlying(m_RepeatEvent.KeyCode);
    const auto device = m_Devices.find(m_RepeatEvent.SourceID);
    if (!m_Focused || device == m_Devices.end() || !device->second.Pressed[keyIndex]) {
        return;
    }
    const xkb_keycode_t keycode = m_PhysicalKeyMap[keyIndex];
    const xkb_keysym_t keysym = xkb_state_key_get_one_sym(m_State.get(), keycode);
    bool composed = false;
    PString text = m_RepeatComposed ? m_RepeatText : TranslateText(keycode, keysym, false, composed);
    if ((GetQualifiers() & (QUAL_CTRL | QUAL_SUPER | QUAL_ALT)) != 0) {
        text.clear();
    }
    PKeyEvent translated = MakeKeyEvent(m_RepeatEvent, device->second.KeySymbols[keyIndex]);
    translated.Timestamp = get_monotonic_time();
    translated.Qualifiers |= QUAL_REPEAT;
    SignalKeyEvent(translated, text);
    timer->Set(TimeValNanos::FromSeconds(KEYREPEAT_REPEAT), true);
    timer->Start(true, &m_Looper);
}
