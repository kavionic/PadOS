// This file is part of PadOS.
//
// Copyright (c) 2018-2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once


class SignalTarget;
class SignalBase;


class SlotBase
{
public:
    SlotBase(SignalBase* targetSignal, SignalTarget* object);
    virtual ~SlotBase();

    virtual SlotBase* Clone(SignalBase* targetSignal) = 0;
    
    SignalTarget* GetSignalTarget() const { return m_Object; }
    SlotBase*     GetNextInSignal() { return m_NextInSignal; }
    SlotBase*     GetPrevInSignal() { return m_PrevInSignal; }
    

protected:
    friend class SignalTarget;
    friend class SignalBase;

    SignalBase*   m_Signal;
    SignalTarget* m_Object;

    SlotBase* m_PrevInSignal;
    SlotBase* m_NextInSignal;
    
    SlotBase* m_PrevInTarget;
    SlotBase* m_NextInTarget;
    
private:
    SlotBase(const SlotBase&) = delete;
    SlotBase& operator=(const SlotBase&) = delete;
};

struct signal_slot_handle_t
{
    signal_slot_handle_t() = default;
    explicit constexpr signal_slot_handle_t(const SlotBase* slot) : SlotPtr(slot) {}

    constexpr bool operator==(const signal_slot_handle_t& rhs) { return SlotPtr == rhs.SlotPtr; }

    constexpr bool operator<(const signal_slot_handle_t& rhs) { return SlotPtr < rhs.SlotPtr; }

private:
    friend class SignalBase;

    const SlotBase* SlotPtr = nullptr; // Only use for lookup. Could be stale.
};
