// This file is part of PadOS.
//
// Copyright (C) 2018 Kurt Skauen <http://kavionic.com/>
//
// PadOS is free software : you can redistribute it and / or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// PadOS is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with PadOS. If not, see <http://www.gnu.org/licenses/>.
///////////////////////////////////////////////////////////////////////////////

#pragma once


class SignalTarget;
class SignalBase;


class SlotBase
{
public:
    SlotBase(SignalBase* targetSignal, SignalTarget* object);
    virtual ~SlotBase();

//    virtual void* GetCallbackAddress() const = 0;
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
