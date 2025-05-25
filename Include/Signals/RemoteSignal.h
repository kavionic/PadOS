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

namespace os
{


namespace remote_signal_utils
{
    constexpr size_t align_argument_size(size_t length) { return (length + 3) & ~3; }

    ///////////////////////////////////////////////////////////////////////////////
    /// \author Kurt Skauen
    ///////////////////////////////////////////////////////////////////////////////

    template<typename T>
    struct ArgumentPacker
    {
        static size_t   GetSize(T value) { return sizeof(T); }
        static ssize_t  Write(T value, void* data, size_t length)
        {
            if (length >= sizeof(value))
            {
                *reinterpret_cast<T*>(data) = value;
                return sizeof(value);
            }
            printf("ERROR: %s: not enough data %zd/%zd.", __PRETTY_FUNCTION__, length, sizeof(T));
            return -1;
        }
        static ssize_t Read(const void* data, size_t length, T* value)
        {
            if (length >= sizeof(T))
            {
                *value = *reinterpret_cast<const T*>(data);
                return sizeof(T);
            }
            printf("ERROR: %s: not enough data %zd/%zd.", __PRETTY_FUNCTION__, length, sizeof(T));
            return -1;
        }
    };

    ///////////////////////////////////////////////////////////////////////////////
    /// \author Kurt Skauen
    ///////////////////////////////////////////////////////////////////////////////

    template<>
    struct ArgumentPacker<std::string>
    {
        static size_t   GetSize(const std::string& value) { return sizeof(uint32_t) + value.size(); }
        static ssize_t  Write(const std::string& value, void* data, size_t length)
        {
            if (length >= (sizeof(uint32_t) + value.size()))
            {
                *reinterpret_cast<uint32_t*>(data) = value.size();
                data = reinterpret_cast<uint32_t*>(data) + 1;
                value.copy(reinterpret_cast<char*>(data), value.size());
                return sizeof(uint32_t) + value.size();
            }
            return -1;
        }
        static ssize_t Read(const void* data, size_t length, std::string* value)
        {
            if (length < sizeof(uint32_t))
            {
                printf("ERROR: %s: not enough data %zd.", __PRETTY_FUNCTION__, length);
                return -1;
            }
            const uint32_t strLength = *reinterpret_cast<const uint32_t*>(data);

            if (length < (sizeof(uint32_t) + strLength))
            {
                printf("ERROR: %s: not enough data %zd / %" PRIu32 ".", __PRETTY_FUNCTION__, length, strLength);
                return -1;
            }

            data = reinterpret_cast<const uint32_t*>(data) + 1;
            value->assign(reinterpret_cast<const char*>(data), strLength);

            return sizeof(uint32_t) + strLength;
        }
    };

    ///////////////////////////////////////////////////////////////////////////////
    /// \author Kurt Skauen
    ///////////////////////////////////////////////////////////////////////////////

    template<>
    struct ArgumentPacker<String> : public ArgumentPacker<std::string>
    {
    };
} // namespace remote_signal_utils

  
  ///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template<typename R, typename... ARGS>
class RemoteSignalTX : public SignalBase
{
public:
    static size_t AccumulateSize() { return 0; }
        
    template<typename FIRST>
    static size_t AccumulateSize(FIRST&& first) { return remote_signal_utils::align_argument_size(first); }
        
    template<typename FIRST, typename... REST>
    static size_t AccumulateSize(FIRST&& first, REST&&... rest) { return remote_signal_utils::align_argument_size(first) + AccumulateSize<REST...>(std::forward<REST>(rest)...); }

    static ssize_t WriteArg(void* buffer, size_t length) { return 0; }
    
    template<typename FIRST>
    static ssize_t WriteArg(void* buffer, size_t length, FIRST&& first)
    {
        ssize_t result = remote_signal_utils::ArgumentPacker<std::decay_t<FIRST>>::Write(std::forward<FIRST>(first), buffer, length);
        if (result >= 0) {
            return remote_signal_utils::align_argument_size(result);
        }
        return -1;
    }
    template<typename FIRST, typename... REST>
    static ssize_t WriteArg(void* buffer, size_t length, FIRST&& first, REST&&... rest)
    {
        ssize_t result = remote_signal_utils::ArgumentPacker<std::decay_t<FIRST>>::Write(std::forward<FIRST>(first), buffer, length);
        if (result >= 0)
        {
            const size_t consumed = remote_signal_utils::align_argument_size(result);
            result = WriteArg(reinterpret_cast<uint8_t*>(buffer) + consumed, length - consumed, std::forward<REST>(rest)...);
            return (result >= 0) ? (result + consumed) : -1;
        }
        return -1;
    }
    
    static size_t GetSize(ARGS... args) { return AccumulateSize(remote_signal_utils::ArgumentPacker<std::decay_t<ARGS>>::GetSize(args)...); }

    void operator()(void* buffer, size_t length, ARGS... args)
    {
        WriteArg(buffer, length, args...);
    }
    
    template<typename CB_OBJ>
    static bool Emit(CB_OBJ* callbackObj, void* (CB_OBJ::*callback)(int32_t, size_t), int32_t messageID, ARGS... args)
    {
        size_t size = AccumulateSize(remote_signal_utils::ArgumentPacker<std::decay_t<ARGS>>::GetSize(args)...);
        
        void* buffer = (callbackObj->*callback)(messageID, size);
        if (buffer == nullptr) {
            return false;
        }
        WriteArg(buffer, size, args...);
        return true;
    }

    static bool Emit(port_id port, handler_id targetHandler, int32_t messageID, const TimeValMicros& timeout, ARGS... args)
    {
        static const size_t MAX_STACK_BUFFER_SIZE = 128;
        
        size_t size = AccumulateSize(remote_signal_utils::ArgumentPacker<std::decay_t<ARGS>>::GetSize(args)...);
        
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
        WriteArg(buffer, size, args...);
        bool result = send_message(port, targetHandler, messageID, buffer, size, timeout.AsMicroSeconds()) >= 0;
        
        if (size > MAX_STACK_BUFFER_SIZE) {
            free(buffer);
        }
        return result;
    }

    static bool Emit(const MessagePort& port, handler_id targetHandler, int32_t messageID, const TimeValMicros& timeout, ARGS... args)
    {
        return Emit(port.GetHandle(), targetHandler, messageID, timeout, args...);
    }
private:
};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template<typename R, typename... ARGS>
class RemoteSignalTXLinked : public RemoteSignalTX<R, ARGS...>
{
public:
    RemoteSignalTXLinked() {}
    
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
        
        size_t size = this->AccumulateSize(remote_signal_utils::ArgumentPacker<std::decay_t<ARGS>>::GetSize(args)...);
        
        void* buffer;
        if (size <= MAX_STACK_BUFFER_SIZE) {
            buffer = alloca(size);
        } else {
            buffer = malloc(size);
            if (buffer == nullptr) {
                return false;
            }
        }
        this->WriteArg(buffer, size, args...);
        bool result = m_TransmitSlot->Call(messageID, buffer, size);
        
        if (size > MAX_STACK_BUFFER_SIZE) {
            free(buffer);
        }
        return result;
    }

private:
    Slot<bool, int, const void*, size_t>* m_TransmitSlot;
};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

class RemoteSignalRXBase
{
public:
    virtual bool Dispatch(const void* data, size_t length) = 0;
};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template<typename R, typename... ARGS>
class RemoteSignalRX : public RemoteSignalRXBase, public Signal<R, ARGS...>
{
public:
    typedef std::tuple<std::decay_t<ARGS>...> ArgTuple_t;
    RemoteSignalRX() {}
    
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
        ssize_t result = remote_signal_utils::ArgumentPacker<FIRST>::Read(data, length, &std::get<I>(tuple));
        if (result >= 0)
        {
            const size_t consumed = remote_signal_utils::align_argument_size(result);
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
class RemoteSignal : public SignalTarget
{
public:
    static int GetID() { return MSGID; }

    typedef RemoteSignalRX<void, ARGS...> Receiver;
    typedef RemoteSignalTX<void, ARGS...> Sender;

    template <typename ...fARGS>
    void Connect(const Signal<void, fARGS...>& srcSignal)
    {
        srcSignal.Connect(this, &RemoteSignal::SlotSignalReceived);
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
    TimeValMicros   m_EmitTimeout = TimeValMicros::infinit;
};

template<int MSGID, typename R, typename... ARGS>
class RemoteSignal<MSGID, R (ARGS...)> : public RemoteSignal<MSGID, ARGS...> {};


template<typename SIGNAL, typename CB_OBJ, typename... ARGS>
bool post_to_remotesignal(CB_OBJ* callbackObj, void* (CB_OBJ::* callback)(int32_t, size_t), ARGS&&... args)
{
    return SIGNAL::Sender::Emit(callbackObj, callback, SIGNAL::GetID(), args...);
}

template<typename SIGNAL, typename... ARGS>
bool post_to_remotesignal(port_id port, handler_id targetHandler, const TimeValMicros& timeout, ARGS&&... args)
{
    return SIGNAL::Sender::Emit(port, targetHandler, SIGNAL::GetID(), timeout, args...);
}

template<typename SIGNAL, typename... ARGS>
bool post_to_remotesignal(const MessagePort& port, handler_id targetHandler, const TimeValMicros& timeout, ARGS&&... args)
{
    return SIGNAL::Sender::Emit(port, targetHandler, SIGNAL::GetID(), timeout, args...);
}

} // namespace
