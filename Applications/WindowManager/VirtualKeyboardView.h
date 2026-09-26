// This file is part of PadOS.
//
// Copyright (c) 2020 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 05.09.2020 22:15

#pragma once

#include <GUI/View.h>


class PTextBox;
class PKeyboardView;

class PVirtualKeyboardView : public PView
{
public:
    PVirtualKeyboardView(bool numerical);
    void SetIsNumerical(bool numerical);
private:
    void SlotKeyPressed(PKeyCodes keyCode, const PString& text);

    Ptr<PKeyboardView>   m_KeyboardView;
};
