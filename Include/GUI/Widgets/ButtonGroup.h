// This file is part of PadOS.
//
// Copyright (c) 2020 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 29.06.2020 19:45

#pragma once

#include <vector>

#include <Ptr/PtrTarget.h>
#include <Signals/SignalTarget.h>
#include <Signals/Signal.h>
#include <Utils/String.h>


class PButtonBase;

class PButtonGroup : public PtrTarget, public SignalTarget
{
public:
    PButtonGroup(const PString& name = PString::zero, size_t reserveCount = 0);
    virtual ~PButtonGroup() override;

    const PString&  GetName() const { return m_Name; }

    size_t          AddButton(Ptr<PButtonBase> button);
    void            InsertButton(size_t index, Ptr<PButtonBase> button);
    Ptr<PButtonBase> RemoveButton(Ptr<PButtonBase> button);
    Ptr<PButtonBase> RemoveButtonAt(size_t index);

    Ptr<PButtonBase> SetSelectedIndex(size_t index, bool sendEvent = true);
    size_t          SelectButton(Ptr<PButtonBase> button, bool sendEvent = true);
    Ptr<PButtonBase> GetSelectedButton() const;
    int32_t         GetSelectedID() const;
    size_t          GetSelectedIndex() const;

    size_t          GetButtonIndex(Ptr<PButtonBase> button) const;

    size_t          GetButtonCount() const;
    Ptr<PButtonBase> GetButton(size_t index) const;

    Signal<void, size_t, int32_t, Ptr<PButtonBase>, Ptr<PButtonGroup>> SignalSelectionChanged;//(size_t index, int32_t ID, Ptr<ButtonBase> button, Ptr<ButtonGroup> group)
private:
    PString                             m_Name;
    std::vector<WeakPtr<PButtonBase>>    m_ButtonList;
};
