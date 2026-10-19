// This file is part of PadOS.
//
// Copyright (c) 2020-2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 17.06.2020 23:45:32


#include <Utils/XMLObjectParser.h>

#include <algorithm>
#include <iterator>

#include <Math/Rect.h>
#include <GUI/LayoutNode.h>
#include <GUI/GUIDefines.h>
#include <Utils/UTF8Utils.h>


namespace p_xml_object_parser
{

bool parse(const char* text, int& value)	{ return sscanf(text, " %d ", &value) == 1; }
bool parse(const char* text, int32_t& value)	{ return sscanf(text, " %ld ", &value) == 1; }
bool parse(const char* text, uint32_t& value)	{ return sscanf(text, " %lu ", &value) == 1; }
bool parse(const char* text, int64_t& value)	{ return sscanf(text, " %lld ", &value) == 1; }
bool parse(const char* text, uint64_t& value)	{ return sscanf(text, " %llu ", &value) == 1; }
bool parse(const char* text, float& value)	{ return sscanf(text, " %f ", &value) == 1; }
bool parse(const char* text, double& value)	{ return sscanf(text, " %lf ", &value) == 1; }

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool parse(const char* text, bool& value)
{
	if (strcmp(text, "false") == 0 || strcmp(text, "0") == 0) {
		value = false;
		return true;
	} else if (strcmp(text, "true") == 0 || strcmp(text, "1") == 0) {
		value = true;
		return true;
	}
	return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool parse(const char* text, PString& value)
{
	value = text; return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool parse(const char* text, PPoint& value)
{
    if (std::optional<PPoint> point = PPoint::FromString(text))
    {
        value = *point;
        return true;
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool parse(const char* text, PRect& value)
{
	return sscanf(text, "Rect( %f , %f , %f , %f )", &value.left, &value.top, &value.right, &value.bottom) == 4;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool parse(const char* text, Ptr<PLayoutNode>& value)
{
    if (*text != '\0')
    {
	if (strcmp(text, "stacked") == 0) {
	    value = ptr_new<PLayoutNode>();
	    return true;
	} else if (strcmp(text, "horizontal") == 0) {
	    value = ptr_new<PHLayoutNode>();
	    return true;
	} else if (strcmp(text, "vertical") == 0) {
	    value = ptr_new<PVLayoutNode>();
	    return true;
	} else {
        p_system_log<PLogSeverity::ERROR>(LogCat_General, "View - invalid layout mode {}", text);
	    value = nullptr;
	    return false;
	}
    }
    else
    {
	value = nullptr;
	return true;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool parse(const char* text, PViewHitMode& value)
{
    if (*text != '\0')
    {
        if (strcmp(text, "HitTest") == 0)
        {
            value = PViewHitMode::HitTest;
            return true;
        }
        else if (strcmp(text, "IgnoreThis") == 0)
        {
            value = PViewHitMode::IgnoreThis;
            return true;
        }
        else if (strcmp(text, "IgnoreRecursive") == 0)
        {
            value = PViewHitMode::IgnoreRecursive;
            return true;
        }
        else
        {
            p_system_log<PLogSeverity::ERROR>(LogCat_General, "View - invalid hit mode '{}'", text);
            value = PViewHitMode::HitTest;
            return false;
        }
    }
    else
    {
        value = PViewHitMode::HitTest;
        return true;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool parse(const char* text, PAlignment& value)
{
	if (*text != '\0')
	{
		if (strcmp(text, "left") == 0) {
			value = PAlignment::Left;
			return true;
		} else if (strcmp(text, "right") == 0) {
			value = PAlignment::Right;
			return true;
		} else if (strcmp(text, "top") == 0) {
			value = PAlignment::Top;
			return true;
		} else if (strcmp(text, "bottom") == 0) {
			value = PAlignment::Bottom;
			return true;
		} else if (strcmp(text, "center") == 0) {
			value = PAlignment::Center;
			return true;
        } else if (strcmp(text, "stretch") == 0) {
            value = PAlignment::Stretch;
            return true;
        } else {
            p_system_log<PLogSeverity::ERROR>(LogCat_General, "View - invalid layout mode '{}'", text);
			value = PAlignment::Left;
			return false;
		}
	}
    else
	{
		value = PAlignment::Left;
		return true;
	}
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool parse(const char* text, POrientation& value)
{
	if (*text != '\0')
	{
		if (strcmp(text, "horizontal") == 0) {
			value = POrientation::Horizontal;
			return true;
		} else if (strcmp(text, "vertical") == 0) {
			value = POrientation::Vertical;
			return true;
		} else {
            p_system_log<PLogSeverity::ERROR>(LogCat_General, "View - invalid orientation '{}'", text);
			value = POrientation::Horizontal;
			return false;
		}
	}
	else
	{
		value = POrientation::Horizontal;
		return true;
	}
}

bool parse(const char* text, PKeyCodes& value)
{
    struct KeyName
    {
        const char* Name;
        PKeyCodes KeyCode;
    };
    static constexpr KeyName KEY_NAMES[] =
    {
        {"A", PKeyCodes::A},
        {"AGAIN", PKeyCodes::AGAIN},
        {"ALTERNATE_ERASE", PKeyCodes::ALTERNATE_ERASE},
        {"APOSTROPHE", PKeyCodes::APOSTROPHE},
        {"APPLICATION", PKeyCodes::APPLICATION},
        {"B", PKeyCodes::B},
        {"BACKSLASH", PKeyCodes::BACKSLASH},
        {"BACKSPACE", PKeyCodes::BACKSPACE},
        {"C", PKeyCodes::C},
        {"CANCEL", PKeyCodes::CANCEL},
        {"CAPS_LOCK", PKeyCodes::CAPS_LOCK},
        {"CLEAR", PKeyCodes::CLEAR},
        {"CLEAR_AGAIN", PKeyCodes::CLEAR_AGAIN},
        {"COMMA", PKeyCodes::COMMA},
        {"COPY", PKeyCodes::COPY},
        {"CURRENCY_SUBUNIT", PKeyCodes::CURRENCY_SUBUNIT},
        {"CURRENCY_UNIT", PKeyCodes::CURRENCY_UNIT},
        {"CURSOR_SELECT", PKeyCodes::CURSOR_SELECT},
        {"CUT", PKeyCodes::CUT},
        {"D", PKeyCodes::D},
        {"DECIMAL_SEPARATOR", PKeyCodes::DECIMAL_SEPARATOR},
        {"DELETE", PKeyCodes::DELETE},
        {"DOWN_ARROW", PKeyCodes::DOWN_ARROW},
        {"E", PKeyCodes::E},
        {"END", PKeyCodes::END},
        {"ENTER", PKeyCodes::ENTER},
        {"EQUAL", PKeyCodes::EQUAL},
        {"ESCAPE", PKeyCodes::ESCAPE},
        {"EXECUTE", PKeyCodes::EXECUTE},
        {"EXTEND_SELECT", PKeyCodes::EXTEND_SELECT},
        {"F", PKeyCodes::F},
        {"F1", PKeyCodes::F1},
        {"F10", PKeyCodes::F10},
        {"F11", PKeyCodes::F11},
        {"F12", PKeyCodes::F12},
        {"F13", PKeyCodes::F13},
        {"F14", PKeyCodes::F14},
        {"F15", PKeyCodes::F15},
        {"F16", PKeyCodes::F16},
        {"F17", PKeyCodes::F17},
        {"F18", PKeyCodes::F18},
        {"F19", PKeyCodes::F19},
        {"F2", PKeyCodes::F2},
        {"F20", PKeyCodes::F20},
        {"F21", PKeyCodes::F21},
        {"F22", PKeyCodes::F22},
        {"F23", PKeyCodes::F23},
        {"F24", PKeyCodes::F24},
        {"F3", PKeyCodes::F3},
        {"F4", PKeyCodes::F4},
        {"F5", PKeyCodes::F5},
        {"F6", PKeyCodes::F6},
        {"F7", PKeyCodes::F7},
        {"F8", PKeyCodes::F8},
        {"F9", PKeyCodes::F9},
        {"FIND", PKeyCodes::FIND},
        {"G", PKeyCodes::G},
        {"GRAVE", PKeyCodes::GRAVE},
        {"H", PKeyCodes::H},
        {"HELP", PKeyCodes::HELP},
        {"HOME", PKeyCodes::HOME},
        {"I", PKeyCodes::I},
        {"INSERT", PKeyCodes::INSERT},
        {"INTERNATIONAL_1", PKeyCodes::INTERNATIONAL_1},
        {"INTERNATIONAL_2", PKeyCodes::INTERNATIONAL_2},
        {"INTERNATIONAL_3", PKeyCodes::INTERNATIONAL_3},
        {"INTERNATIONAL_4", PKeyCodes::INTERNATIONAL_4},
        {"INTERNATIONAL_5", PKeyCodes::INTERNATIONAL_5},
        {"INTERNATIONAL_6", PKeyCodes::INTERNATIONAL_6},
        {"INTERNATIONAL_7", PKeyCodes::INTERNATIONAL_7},
        {"INTERNATIONAL_8", PKeyCodes::INTERNATIONAL_8},
        {"INTERNATIONAL_9", PKeyCodes::INTERNATIONAL_9},
        {"J", PKeyCodes::J},
        {"K", PKeyCodes::K},
        {"KEYPAD_0", PKeyCodes::KEYPAD_0},
        {"KEYPAD_00", PKeyCodes::KEYPAD_00},
        {"KEYPAD_000", PKeyCodes::KEYPAD_000},
        {"KEYPAD_1", PKeyCodes::KEYPAD_1},
        {"KEYPAD_2", PKeyCodes::KEYPAD_2},
        {"KEYPAD_3", PKeyCodes::KEYPAD_3},
        {"KEYPAD_4", PKeyCodes::KEYPAD_4},
        {"KEYPAD_5", PKeyCodes::KEYPAD_5},
        {"KEYPAD_6", PKeyCodes::KEYPAD_6},
        {"KEYPAD_7", PKeyCodes::KEYPAD_7},
        {"KEYPAD_8", PKeyCodes::KEYPAD_8},
        {"KEYPAD_9", PKeyCodes::KEYPAD_9},
        {"KEYPAD_A", PKeyCodes::KEYPAD_A},
        {"KEYPAD_ADD", PKeyCodes::KEYPAD_ADD},
        {"KEYPAD_AMPERSAND", PKeyCodes::KEYPAD_AMPERSAND},
        {"KEYPAD_AT", PKeyCodes::KEYPAD_AT},
        {"KEYPAD_B", PKeyCodes::KEYPAD_B},
        {"KEYPAD_BACKSPACE", PKeyCodes::KEYPAD_BACKSPACE},
        {"KEYPAD_BINARY", PKeyCodes::KEYPAD_BINARY},
        {"KEYPAD_C", PKeyCodes::KEYPAD_C},
        {"KEYPAD_CARET", PKeyCodes::KEYPAD_CARET},
        {"KEYPAD_CLEAR", PKeyCodes::KEYPAD_CLEAR},
        {"KEYPAD_CLEAR_ENTRY", PKeyCodes::KEYPAD_CLEAR_ENTRY},
        {"KEYPAD_COLON", PKeyCodes::KEYPAD_COLON},
        {"KEYPAD_COMMA", PKeyCodes::KEYPAD_COMMA},
        {"KEYPAD_D", PKeyCodes::KEYPAD_D},
        {"KEYPAD_DECIMAL", PKeyCodes::KEYPAD_DECIMAL},
        {"KEYPAD_DIVIDE", PKeyCodes::KEYPAD_DIVIDE},
        {"KEYPAD_DOUBLE_AMPERSAND", PKeyCodes::KEYPAD_DOUBLE_AMPERSAND},
        {"KEYPAD_DOUBLE_VERTICAL_BAR", PKeyCodes::KEYPAD_DOUBLE_VERTICAL_BAR},
        {"KEYPAD_E", PKeyCodes::KEYPAD_E},
        {"KEYPAD_ENTER", PKeyCodes::KEYPAD_ENTER},
        {"KEYPAD_EQUAL", PKeyCodes::KEYPAD_EQUAL},
        {"KEYPAD_EQUAL_AS400", PKeyCodes::KEYPAD_EQUAL_AS400},
        {"KEYPAD_EXCLAMATION", PKeyCodes::KEYPAD_EXCLAMATION},
        {"KEYPAD_F", PKeyCodes::KEYPAD_F},
        {"KEYPAD_GREATER_THAN", PKeyCodes::KEYPAD_GREATER_THAN},
        {"KEYPAD_HASH", PKeyCodes::KEYPAD_HASH},
        {"KEYPAD_HEXADECIMAL", PKeyCodes::KEYPAD_HEXADECIMAL},
        {"KEYPAD_LEFT_BRACE", PKeyCodes::KEYPAD_LEFT_BRACE},
        {"KEYPAD_LEFT_PARENTHESIS", PKeyCodes::KEYPAD_LEFT_PARENTHESIS},
        {"KEYPAD_LESS_THAN", PKeyCodes::KEYPAD_LESS_THAN},
        {"KEYPAD_MEMORY_ADD", PKeyCodes::KEYPAD_MEMORY_ADD},
        {"KEYPAD_MEMORY_CLEAR", PKeyCodes::KEYPAD_MEMORY_CLEAR},
        {"KEYPAD_MEMORY_DIVIDE", PKeyCodes::KEYPAD_MEMORY_DIVIDE},
        {"KEYPAD_MEMORY_MULTIPLY", PKeyCodes::KEYPAD_MEMORY_MULTIPLY},
        {"KEYPAD_MEMORY_RECALL", PKeyCodes::KEYPAD_MEMORY_RECALL},
        {"KEYPAD_MEMORY_STORE", PKeyCodes::KEYPAD_MEMORY_STORE},
        {"KEYPAD_MEMORY_SUBTRACT", PKeyCodes::KEYPAD_MEMORY_SUBTRACT},
        {"KEYPAD_MULTIPLY", PKeyCodes::KEYPAD_MULTIPLY},
        {"KEYPAD_OCTAL", PKeyCodes::KEYPAD_OCTAL},
        {"KEYPAD_PERCENT", PKeyCodes::KEYPAD_PERCENT},
        {"KEYPAD_PERIOD", PKeyCodes::KEYPAD_PERIOD},
        {"KEYPAD_PLUS_MINUS", PKeyCodes::KEYPAD_PLUS_MINUS},
        {"KEYPAD_RIGHT_BRACE", PKeyCodes::KEYPAD_RIGHT_BRACE},
        {"KEYPAD_RIGHT_PARENTHESIS", PKeyCodes::KEYPAD_RIGHT_PARENTHESIS},
        {"KEYPAD_SPACE", PKeyCodes::KEYPAD_SPACE},
        {"KEYPAD_SUBTRACT", PKeyCodes::KEYPAD_SUBTRACT},
        {"KEYPAD_TAB", PKeyCodes::KEYPAD_TAB},
        {"KEYPAD_VERTICAL_BAR", PKeyCodes::KEYPAD_VERTICAL_BAR},
        {"KEYPAD_XOR", PKeyCodes::KEYPAD_XOR},
        {"L", PKeyCodes::L},
        {"LANGUAGE_1", PKeyCodes::LANGUAGE_1},
        {"LANGUAGE_2", PKeyCodes::LANGUAGE_2},
        {"LANGUAGE_3", PKeyCodes::LANGUAGE_3},
        {"LANGUAGE_4", PKeyCodes::LANGUAGE_4},
        {"LANGUAGE_5", PKeyCodes::LANGUAGE_5},
        {"LANGUAGE_6", PKeyCodes::LANGUAGE_6},
        {"LANGUAGE_7", PKeyCodes::LANGUAGE_7},
        {"LANGUAGE_8", PKeyCodes::LANGUAGE_8},
        {"LANGUAGE_9", PKeyCodes::LANGUAGE_9},
        {"LEFT_ALT", PKeyCodes::LEFT_ALT},
        {"LEFT_ARROW", PKeyCodes::LEFT_ARROW},
        {"LEFT_BRACKET", PKeyCodes::LEFT_BRACKET},
        {"LEFT_CONTROL", PKeyCodes::LEFT_CONTROL},
        {"LEFT_GUI", PKeyCodes::LEFT_GUI},
        {"LEFT_SHIFT", PKeyCodes::LEFT_SHIFT},
        {"LOCKING_CAPS_LOCK", PKeyCodes::LOCKING_CAPS_LOCK},
        {"LOCKING_NUM_LOCK", PKeyCodes::LOCKING_NUM_LOCK},
        {"LOCKING_SCROLL_LOCK", PKeyCodes::LOCKING_SCROLL_LOCK},
        {"M", PKeyCodes::M},
        {"MENU", PKeyCodes::MENU},
        {"MINUS", PKeyCodes::MINUS},
        {"MUTE", PKeyCodes::MUTE},
        {"N", PKeyCodes::N},
        {"NONE", PKeyCodes::NONE},
        {"NON_US_BACKSLASH", PKeyCodes::NON_US_BACKSLASH},
        {"NON_US_HASH", PKeyCodes::NON_US_HASH},
        {"NUM_0", PKeyCodes::NUM_0},
        {"NUM_1", PKeyCodes::NUM_1},
        {"NUM_2", PKeyCodes::NUM_2},
        {"NUM_3", PKeyCodes::NUM_3},
        {"NUM_4", PKeyCodes::NUM_4},
        {"NUM_5", PKeyCodes::NUM_5},
        {"NUM_6", PKeyCodes::NUM_6},
        {"NUM_7", PKeyCodes::NUM_7},
        {"NUM_8", PKeyCodes::NUM_8},
        {"NUM_9", PKeyCodes::NUM_9},
        {"NUM_LOCK", PKeyCodes::NUM_LOCK},
        {"O", PKeyCodes::O},
        {"OPER", PKeyCodes::OPER},
        {"OUT", PKeyCodes::OUT},
        {"P", PKeyCodes::P},
        {"PAGE_DOWN", PKeyCodes::PAGE_DOWN},
        {"PAGE_UP", PKeyCodes::PAGE_UP},
        {"PASTE", PKeyCodes::PASTE},
        {"PAUSE", PKeyCodes::PAUSE},
        {"PERIOD", PKeyCodes::PERIOD},
        {"POWER", PKeyCodes::POWER},
        {"PRINT_SCREEN", PKeyCodes::PRINT_SCREEN},
        {"PRIOR", PKeyCodes::PRIOR},
        {"Q", PKeyCodes::Q},
        {"R", PKeyCodes::R},
        {"RETURN", PKeyCodes::RETURN},
        {"RIGHT_ALT", PKeyCodes::RIGHT_ALT},
        {"RIGHT_ARROW", PKeyCodes::RIGHT_ARROW},
        {"RIGHT_BRACKET", PKeyCodes::RIGHT_BRACKET},
        {"RIGHT_CONTROL", PKeyCodes::RIGHT_CONTROL},
        {"RIGHT_GUI", PKeyCodes::RIGHT_GUI},
        {"RIGHT_SHIFT", PKeyCodes::RIGHT_SHIFT},
        {"S", PKeyCodes::S},
        {"SCROLL_LOCK", PKeyCodes::SCROLL_LOCK},
        {"SELECT", PKeyCodes::SELECT},
        {"SEMICOLON", PKeyCodes::SEMICOLON},
        {"SEPARATOR", PKeyCodes::SEPARATOR},
        {"SLASH", PKeyCodes::SLASH},
        {"SPACE", PKeyCodes::SPACE},
        {"STOP", PKeyCodes::STOP},
        {"SYSTEM_REQUEST", PKeyCodes::SYSTEM_REQUEST},
        {"T", PKeyCodes::T},
        {"TAB", PKeyCodes::TAB},
        {"THOUSANDS_SEPARATOR", PKeyCodes::THOUSANDS_SEPARATOR},
        {"U", PKeyCodes::U},
        {"UNDO", PKeyCodes::UNDO},
        {"UP_ARROW", PKeyCodes::UP_ARROW},
        {"V", PKeyCodes::V},
        {"VOLUME_DOWN", PKeyCodes::VOLUME_DOWN},
        {"VOLUME_UP", PKeyCodes::VOLUME_UP},
        {"W", PKeyCodes::W},
        {"X", PKeyCodes::X},
        {"Y", PKeyCodes::Y},
        {"Z", PKeyCodes::Z}
    };
    const auto match = std::lower_bound(
        std::begin(KEY_NAMES),
        std::end(KEY_NAMES),
        text,
        [](const KeyName& entry, const char* name) { return strcmp(entry.Name, name) < 0; });
    const bool found = match != std::end(KEY_NAMES) && strcmp(match->Name, text) == 0;
    if (found) {
        value = match->KeyCode;
    }
    return found;
}

}
