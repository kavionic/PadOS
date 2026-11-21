// This file is part of PadOS.
//
// Copyright (c) 2020 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 28.06.2020 12:56

#pragma once

#include <GUI/Style/ButtonStyleData.h>
#include <GUI/Style/ViewStyleButtonBase.h>
#include <GUI/Widgets/Control.h>


class PButtonGroup;

class PButtonBase : public PControl, public PViewStyleButtonBase
{
public:
    PButtonBase(const PString& name = PString::zero, PViewFlags flags = PViewFlags::zero);
    PButtonBase(PViewFactoryContext& context, const pugi::xml_node& xmlData, PAlignment defaultLabelAlignment);
    ~PButtonBase();

    virtual const PButtonStyleData& GetButtonStyleData() const override { return m_ButtonStyleData; }
    virtual bool IsButtonEnabled() const override { return IsEnabled(); }

    static Ptr<PButtonGroup> FindButtonGroup(Ptr<PView> root, const PString& name);

    virtual void AllAttachedToScreen() override { Invalidate(); }
    virtual void OnFontChanged(Ptr<const PFont> newFont) override;
    virtual void OnFrameSized(const PPoint& delta) override;

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

    void SetCheckable(bool value);
    bool IsCheckable() const { return m_ButtonStyleData.IsCheckable; }

    void SetChecked(bool isChecked);
    bool IsChecked() const { return m_ButtonStyleData.IsChecked; }

    // From Control:
    virtual void OnEnableStatusChanged(bool isEnabled) override;

    // From ButtonBase:
    virtual void OnLabelChanged(const PString& label) {}
    virtual void OnPressedStateChanged(bool isPressed) {}
    virtual void OnCheckedStateChanged(bool isChecked) {}

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
    void NotifyRendererPropertyChanged(PButtonProperties properties);
    PPointerID           m_HitPointerID = PInvalidPointerID;
    Ptr<PButtonGroup>        m_ButtonGroup;


    PButtonBase(const PButtonBase&) = delete;
    PButtonBase& operator=(const PButtonBase&) = delete;
};
