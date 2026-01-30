// This file is part of PadOS.
//
// Copyright (C) 2018-2025 Kurt Skauen <http://kavionic.com/>
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

#include <stddef.h>
#include <string>

#include "Signal.h"
#include "Slot.h"
#include "Math/Rect.h"
#include "Utils/MessagePort.h"
#include "Utils/String.h"
#include <RPC/ArgumentPacker.h>
#include <RPC/ArgumentSerializer.h>

 
  ///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template<typename R, typename... ARGS>
class PRemoteSignalTX : public SignalBase
{
public:
    using Serializer = PArgumentSerializer<R, ARGS...>;
    
    template<typename CB_OBJ>
    static bool Emit(CB_OBJ* callbackObj, void* (CB_OBJ::*callback)(int32_t, size_t), int32_t messageID, ARGS... args)
    {
        size_t size = Serializer::AccumulateSize(PArgumentPacker<std::decay_t<ARGS>>::GetSize(args)...);
        
        void* buffer = (callbackObj->*callback)(messageID, size);
        if (buffer == nullptr) {
            return false;
        }
        Serializer::WriteArg(buffer, size, args...);
        return true;
    }

    static bool Emit(port_id port, handler_id targetHandler, int32_t messageID, const TimeValNanos& timeout, ARGS... args)
    {
        static const size_t MAX_STACK_BUFFER_SIZE = 128;
        
        size_t size = Serializer::AccumulateSize(PArgumentPacker<std::decay_t<ARGS>>::GetSize(args)...);
        
        void* buffer;

        if (size == 0)
        {
            buffer = nullptr;
        }
        else if (size <= MAX_STACK_BUFFER_SIZE)
        {
            buffer = alloca(size);
        }
        else
        {
            buffer = malloc(size);
            if (buffer == nullptr) {
                return false;
            }
        }
        Serializer::WriteArg(buffer, size, args...);
        const PErrorCode result = message_port_send_timeout_ns(port, targetHandler, messageID, buffer, size, timeout.AsNanoseconds());

        if (result != PErrorCode::Success) {
            set_last_error(result);
        }
        
        if (size > MAX_STACK_BUFFER_SIZE) {
            free(buffer);
        }
        return result == PErrorCode::Success;
    }

    static bool Emit(const PMessagePort& port, handler_id targetHandler, int32_t messageID, const TimeValNanos& timeout, ARGS... args)
    {
        return Emit(port.GetHandle(), targetHandler, messageID, timeout, args...);
    }
private:
};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template<typename R, typename... ARGS>
class PRemoteSignalTXLinked : public PRemoteSignalTX<R, ARGS...>
{
public:
    PRemoteSignalTXLinked() {}
    
    template <typename T,typename fT>
    bool SetTransmitter(const T* object, bool (fT::*callback)(int, const void*, size_t))
    {
        try {
            typedef bool (fT::*Signature)(int, const void*, size_t);
            m_TransmitSlot = new SlotFull<3, fT, bool, Signature, int, const void*, size_t>(this, const_cast<fT*>(static_cast<const fT*>(object)), callback);
            return true;
        } catch (const std::bad_alloc& error) {
            return false;
        }
    }
    template <typename T,typename fT>
    bool SetTransmitter(const T* object, bool (fT::*callback)(int, const void*, size_t) const)
    {
        try {
            typedef bool (fT::*Signature)(int, const void*, size_t) const;
            m_TransmitSlot = new SlotFull<3, fT, bool, Signature, int, const void*, size_t>(this, const_cast<fT*>(static_cast<const fT*>(object)), callback);
            return true;
        } catch (const std::bad_alloc& error) {
            return false;
        }
    }
    bool SetTransmitter(bool (*callback)(int, const void*, size_t))
    {
        try {
            typedef bool (*Signature)(int, const void*, size_t);
            m_TransmitSlot = new SlotFull<3, SignalTarget, bool, Signature, int, const void*, size_t>(this, nullptr, callback);
            return true;
        } catch (const std::bad_alloc& error) {
            return false;
        }
    }

    bool operator()(int32_t messageID, ARGS... args)
    {
        static const size_t MAX_STACK_BUFFER_SIZE = 128;
        
        constexpr size_t size = this->AccumulateSize(PArgumentPacker<std::decay_t<ARGS>>::GetSize(args)...);
        
        void* buffer;
        std::vector<uint8_t> heapBuffer;
        if (size <= MAX_STACK_BUFFER_SIZE)
        {
            buffer = alloca(size);
        }
        else
        {
            heapBuffer.resize(size);
            buffer = heapBuffer.data();
        }
        this->WriteArg(buffer, size, args...);
        bool result = m_TransmitSlot->Call(messageID, buffer, size);
        
        return result;
    }

private:
    Slot<bool, int, const void*, size_t>* m_TransmitSlot;
};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

class PRemoteSignalRXBase
{
public:
    virtual bool Dispatch(const void* data, size_t length) = 0;
};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template<typename R, typename... ARGS>
class PRemoteSignalRX : public PRemoteSignalRXBase, public Signal<R, ARGS...>
{
public:
    using ReturnType = R;

    typedef std::tuple<std::decay_t<ARGS>...> ArgTuple_t;
    PRemoteSignalRX() {}
    
    virtual bool Dispatch(const void* data, size_t length) override
    {
        using Indices = std::make_index_sequence<std::tuple_size<std::decay_t<ArgTuple_t>>::value>;
        SendSignal(data, length, Indices());
        return true;
    }
        
private:
    template<int I>
    ssize_t UnpackArgs(ArgTuple_t& tuple, const void* data, size_t length) { return 0; }
    
    template<int I, typename FIRST, typename... REST>
    ssize_t UnpackArgs(ArgTuple_t& tuple, const void* data, size_t length)
    {
        ssize_t result = PArgumentPacker<FIRST>::Read(data, length, &std::get<I>(tuple));
        if (result >= 0)
        {
            const size_t consumed = align_argument_size(result);
            data = reinterpret_cast<const uint8_t*>(data) + consumed;
            result = UnpackArgs<I + 1, REST...>(tuple, data, length - consumed);
            if (result >= 0)
            {
                return result + consumed;
            }
        }
        return -1;
    }

    template<std::size_t... I>
    void SendSignal(const void* data, size_t length, std::index_sequence<I...>)
    {
        ArgTuple_t argPack;
        if (UnpackArgs<0, std::decay_t<ARGS>...>(argPack, data, length) >= 0) {
            (*this)(std::get<I>(argPack)...);
        }
    }
};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template<int MSGID, typename... ARGS>
class PRemoteSignal : public SignalTarget
{
public:
    static int GetID() { return MSGID; }

    typedef PRemoteSignalRX<void, ARGS...> Receiver;
    typedef PRemoteSignalTX<void, ARGS...> Sender;

    template <typename ...fARGS>
    void Connect(const Signal<void, fARGS...>& srcSignal)
    {
        srcSignal.Connect(this, &PRemoteSignal::SlotSignalReceived);
    }

    template <typename fR, typename fC, typename T, typename ...fARGS>
    void Connect(const T* object, fR(fC::* callback)(fARGS...)) const
    {
        ReceiverObj.Connect(object, callback);
    }

    void SetRemoteTarget(port_id targetPort, handle_id targetHandler)
    {
        m_TargetPort = targetPort;
        m_TargetHandler = targetHandler;
    }

    Receiver    ReceiverObj;
    Sender      SenderObj;

private:
    void SlotSignalReceived(ARGS ...args)
    {
        SenderObj.Emit(m_TargetPort, m_TargetHandler, MSGID, m_EmitTimeout, args...);
    }

    port_id         m_TargetPort = INVALID_HANDLE;
    handle_id       m_TargetHandler = INVALID_HANDLE;
    TimeValNanos    m_EmitTimeout = TimeValNanos::infinit;
};

template<int MSGID, typename R, typename... ARGS>
class PRemoteSignal<MSGID, R (ARGS...)> : public PRemoteSignal<MSGID, ARGS...> {};


template<typename SIGNAL, typename CB_OBJ, typename... ARGS>
bool p_post_to_remotesignal(CB_OBJ* callbackObj, void* (CB_OBJ::* callback)(int32_t, size_t), ARGS&&... args)
{
    return SIGNAL::Sender::Emit(callbackObj, callback, SIGNAL::GetID(), args...);
}

template<typename SIGNAL, typename... ARGS>
bool p_post_to_remotesignal(port_id port, handler_id targetHandler, const TimeValNanos& timeout, ARGS&&... args)
{
    return SIGNAL::Sender::Emit(port, targetHandler, SIGNAL::GetID(), timeout, args...);
}

template<typename SIGNAL, typename... ARGS>
bool p_post_to_remotesignal(const PMessagePort& port, handler_id targetHandler, const TimeValNanos& timeout, ARGS&&... args)
{
    return SIGNAL::Sender::Emit(port, targetHandler, SIGNAL::GetID(), timeout, args...);
}
