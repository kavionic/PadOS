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

#include <stddef.h>
#include <string>

#include "Signal.h"
#include "Slot.h"
#include "Math/Rect.h"
#include "Utils/MessagePort.h"
#include "Utils/String.h"

namespace os
{

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template<typename T>
struct RemoteSignalPacker
{
    static size_t GetSize(T value) { return sizeof(T); }
    static void   Write(T value, void* data ) { *reinterpret_cast<T*>(data) = value; }
    static size_t Read(const void* data, T* value)       { *value = *reinterpret_cast<const T*>(data); return sizeof(T); }
};
/*
template<>
struct RemoteSignalPacker<int>
{
    static size_t GetSize(int value) { return sizeof(int); }
    static void   Write(int value, void* data ) { *reinterpret_cast<int*>(data) = value; }
    static size_t Read(const void* data, int* value)       { *value = *reinterpret_cast<const int*>(data); return sizeof(*value); }
};


template<>
struct RemoteSignalPacker<float>
{
    static size_t GetSize(float value)           { return sizeof(float); }
    static void   Write(float value, void* data ) { *reinterpret_cast<float*>(data) = value; }
    static size_t Read(const void* data, float* value)       { *value = *reinterpret_cast<const float*>(data); return sizeof(*value); }
};

template<>
struct RemoteSignalPacker<Rect>
{
    static size_t GetSize(const Rect& value)           { return sizeof(Rect); }
    static void   Write(const Rect& value, void* data ) { *reinterpret_cast<Rect*>(data) = value; }
    static size_t Read(const void* data, Rect* value)       { *value = *reinterpret_cast<const Rect*>(data); return sizeof(*value); }
};
*/

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template<>
struct RemoteSignalPacker<std::string>
{
    static size_t GetSize(const std::string& value) { return sizeof(uint32_t) + value.size(); }
    static void   Write(const std::string& value, void* data)
    {
        *reinterpret_cast<uint32_t*>(data) = value.size();
        data = reinterpret_cast<uint32_t*>(data) + 1;
        value.copy(reinterpret_cast<char*>(data), value.size());
    }
    static size_t Read(const void* data, std::string* value)
    {
        uint32_t length = *reinterpret_cast<const uint32_t*>(data);
        data = reinterpret_cast<const uint32_t*>(data) + 1;
        value->assign(reinterpret_cast<const char*>(data), length);
        return sizeof(uint32_t) + length;
    }
};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template<>
struct RemoteSignalPacker<String>
{
    static size_t GetSize(const String& value) { return sizeof(uint32_t) + value.size(); }
    static void   Write(const String& value, void* data)
    {
        *reinterpret_cast<uint32_t*>(data) = value.size();
        data = reinterpret_cast<uint32_t*>(data) + 1;
        value.copy(reinterpret_cast<char*>(data), value.size());
    }
    static size_t Read(const void* data, String* value)
    {
        uint32_t length = *reinterpret_cast<const uint32_t*>(data);
        data = reinterpret_cast<const uint32_t*>(data) + 1;
        value->assign(reinterpret_cast<const char*>(data), length);
        return sizeof(uint32_t) + length;
    }
};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template<int ID, typename R, typename... ARGS>
class RemoteSignalTX : public SignalBase
{
public:
    static int GetID() { return ID; }
    static size_t AccumulateSize() { return 0; }
        
    template<typename FIRST>
    static size_t AccumulateSize(FIRST&& first) { return ((first + 3) & ~3); }
        
    template<typename FIRST, typename... REST>
    static size_t AccumulateSize(FIRST&& first, REST&&... rest) { return ((first + 3) & ~3) + AccumulateSize<REST...>(std::forward<REST>(rest)...); }

    static void WriteArg(void* buffer) {}
    
    template<typename FIRST>
    static void WriteArg(void* buffer, FIRST&& first)
    {
        RemoteSignalPacker<std::decay_t<FIRST>>::Write(std::forward<FIRST>(first), buffer);
    }
    template<typename FIRST, typename... REST>
    static void WriteArg(void* buffer, FIRST&& first, REST&&... rest)
    {
        RemoteSignalPacker<std::decay_t<FIRST>>::Write(std::forward<FIRST>(first), buffer);
        WriteArg(reinterpret_cast<uint8_t*>(buffer) + ((RemoteSignalPacker<std::decay_t<FIRST>>::GetSize(std::forward<FIRST>(first)) + 3) & ~3), std::forward<REST>(rest)...);
    }
    
    static size_t GetSize(ARGS... args) { return AccumulateSize(RemoteSignalPacker<std::decay_t<ARGS>>::GetSize(args)...); }

    void operator()(void* buffer, ARGS... args)
    {
        WriteArg(buffer, args...);
    }
    
    template<typename CB_OBJ>
    static bool Emit(CB_OBJ* callbackObj, void* (CB_OBJ::*callback)(int32_t, size_t), ARGS... args)
    {
        size_t size = AccumulateSize(RemoteSignalPacker<std::decay_t<ARGS>>::GetSize(args)...);
        
        void* buffer = (callbackObj->*callback)(ID, size);
        if (buffer == nullptr) {
            return false;
        }
        WriteArg(buffer, args...);
        return true;
    }

    static bool Emit(port_id port, handler_id targetHandler, bigtime_t timeout, ARGS... args)
    {
        static const size_t MAX_STACK_BUFFER_SIZE = 128;
        
        size_t size = AccumulateSize(RemoteSignalPacker<std::decay_t<ARGS>>::GetSize(args)...);
        
        void* buffer;
        if (size <= MAX_STACK_BUFFER_SIZE) {
            buffer = alloca(size);
        } else {
            buffer = malloc(size);
            if (buffer == nullptr) {
                return false;
            }
        }
        WriteArg(buffer, args...);
        bool result = send_message(port, targetHandler, ID, buffer, size, timeout) >= 0;
        
        if (size > MAX_STACK_BUFFER_SIZE) {
            free(buffer);
        }
        return result;
    }

    static bool Emit(const MessagePort& port, handler_id targetHandler, bigtime_t timeout, ARGS... args)
    {
        return Emit(port.GetPortID(), targetHandler, timeout, args...);
    }
private:
};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template<int ID, typename R, typename... ARGS>
class RemoteSignalTXLinked : public RemoteSignalTX<ID, R, ARGS...>
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

    bool operator()(ARGS... args)
    {
        static const size_t MAX_STACK_BUFFER_SIZE = 128;
        
        size_t size = this->AccumulateSize(RemoteSignalPacker<std::decay_t<ARGS>>::GetSize(args)...);
        
        void* buffer;
        if (size <= MAX_STACK_BUFFER_SIZE) {
            buffer = alloca(size);
        } else {
            buffer = malloc(size);
            if (buffer == nullptr) {
                return false;
            }
        }
        this->WriteArg(buffer, args...);
        bool result = m_TransmitSlot->Call(ID, buffer, size);
        
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
    
    virtual int GetID() const = 0;
};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template<int ID, typename R, typename... ARGS>
class RemoteSignalRX : public RemoteSignalRXBase, public Signal<R, ARGS...>
{
public:
    typedef std::tuple<std::decay_t<ARGS>...> ArgTuple_t;
    RemoteSignalRX() {}
    
    virtual int GetID() const override { return ID; }
    
    virtual bool Dispatch(const void* data, size_t length) override
    {
        using Indices = std::make_index_sequence<std::tuple_size<std::decay_t<ArgTuple_t>>::value>;
        SendSignal(data, Indices());
        return true;
    }
        
private:
    template<int I>
    void UnpackArgs(ArgTuple_t& tuple, const void* data) {}
    
    template<int I, typename FIRST, typename... REST>
    void UnpackArgs(ArgTuple_t& tuple, const void* data)
    {
        data = reinterpret_cast<const uint8_t*>(data) + ((RemoteSignalPacker<FIRST>::Read(data, &std::get<I>(tuple)) + 3) & ~3);
        UnpackArgs<I+1, REST...>(tuple, data);
    }

    template<std::size_t... I>
    void SendSignal(const void* data, std::index_sequence<I...>)
    {
        ArgTuple_t argPack;
        UnpackArgs<0, std::decay_t<ARGS>...>(argPack, data);
        (*this)(std::get<I>(argPack)...);
    }
};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template<int ID, typename... ARGS>
class RemoteSignal
{
public:
    typedef RemoteSignalRX<ID, void, ARGS...> Receiver;    
    typedef RemoteSignalTX<ID, void, ARGS...> Sender;
};

} // namespace
