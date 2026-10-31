// This file is part of PadOS.
//
// Copyright (c) 2020 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 28.06.2020 12:56

#pragma once

#include <GUI/Style/ButtonStyleData.h>
#include <GUI/Widgets/Control.h>


class PButtonGroup;

class PButtonBase : public PControl
{
public:
    PButtonBase(const PString& name = PString::zero, uint32_t flags = 0);
    PButtonBase(PViewFactoryContext& context, const pugi::xml_node& xmlData, PAlignment defaultLabelAlignment);
    ~PButtonBase();

    static Ptr<PButtonGroup> FindButtonGroup(Ptr<PView> root, const PString& name);

    virtual void AllAttachedToScreen() override { Invalidate(); }
    virtual void OnFontChanged(Ptr<const PFont> newFont) override;
    virtual void OnStyleChanged() override;

    virtual void OnPointerDown(PPointerID pointerID, const PPoint& position, const PPointerEvent& event, PEventPhase phase) override;
    virtual void OnPointerUp(PPointerID pointerID, const PPoint& position, const PPointerEvent& event, PEventPhase phase) override;
    virtual void OnPointerMove(PPointerID pointerID, const PPoint& position, const PPointerEvent& event, PEventPhase phase) override;
    virtual void OnPointerOver(PPointerID pointerID, const PPoint& position, const PPointerEvent& pointerEvent, PEventPhase phase) override;
    virtual void OnPointerOut(PPointerID pointerID, const PPoint& position, const PPointerEvent& pointerEvent, PEventPhase phase) override;
    virtual void OnPointerCaptureLost(PPointerID pointerID, PPointerCaptureLostReason reason) override;

    void SetLabel(const PString& label);
    const PString& GetLabel() const { return m_ButtonStyleData.Label; }

    void SetLabelAlignment(PAlignment alignment);
    PAlignment GetLabelAlignment() const { return m_ButtonStyleData.LabelAlignment; }

    void SetCheckable(bool value) { m_ButtonStyleData.IsCheckable = value; }
    bool IsCheckable() const { return m_ButtonStyleData.IsCheckable; }

    void SetChecked(bool isChecked);
    bool IsChecked() const { return m_ButtonStyleData.IsChecked; }

    // From Control:
    virtual void OnEnableStatusChanged(bool isEnabled) override;

    // From ButtonBase:
    virtual void OnLabelChanged(const PString& label) { Invalidate(); Flush(); PreferredSizeChanged(); }
    virtual void OnPressedStateChanged(bool isPressed) { Invalidate(); Flush(); }
    virtual void OnCheckedStateChanged(bool isChecked) { Invalidate(); Flush(); }

    Ptr<PButtonGroup> GetButtonGroup() const;

    Signal<void, PPointerID, PButtonBase*>    SignalActivated;
    Signal<void, bool, PButtonBase*>             SignalToggled;

protected:
    void SetPressedState(bool isPressed);
    bool GetPressedState() const { return m_ButtonStyleData.IsPressed; }
    bool IsPointerOver() const { return m_ButtonStyleData.IsPointerOver; }

    PButtonStyleData m_ButtonStyleData;


private:
    friend class PButtonGroup;

    void ActivateCheckableButton();
    void SetButtonGroup(Ptr<PButtonGroup> group);
    PPointerID           m_HitPointerID = PInvalidPointerID;
    Ptr<PButtonGroup>        m_ButtonGroup;


    PButtonBase(const PButtonBase&) = delete;
    PButtonBase& operator=(const PButtonBase&) = delete;
};
