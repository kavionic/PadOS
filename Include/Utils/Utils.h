// This file is part of PadOS.
//
// Copyright (C) 2016-2022 Kurt Skauen <http://kavionic.com/>
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

#include <cmath>
#include <PadOS/Time.h>
#include <System/Types.h>
#include <System/System.h>
#include <Utils/String.h>
#include <Utils/Logging.h>
#include <Kernel/Scheduler.h>

template<typename T>
constexpr int get_first_bit_index(T value) { return (value & 0x01) ? 0 : get_first_bit_index(value >> 1) + 1; }
template<typename T>
constexpr int get_bit_count(T value) { return (value == 0) ? 0 : get_bit_count(value >> 1) + (value & 0x01); }

template<typename T> inline T wrap(const T& bottom, const T& top, const T& value)
{
    if ( value < bottom )
    {
        return top - (bottom - value) + 1;
    } else if ( value > top ) {
        return bottom + (value - top) - 1;
    } else {
        return value;
    }        
}

template<typename Tvalue, typename Talign>
constexpr Tvalue align_up(Tvalue value, Talign alignment) { return (value + alignment - 1) & ~(alignment - 1); }

template<typename Tvalue, typename Talign>
constexpr Tvalue align_down(Tvalue value, Talign alignment) { return value & ~(alignment - 1); }


template<typename T>
T* add_bytes_to_pointer(T* pointer, ssize_t adder) { return reinterpret_cast<T*>(reinterpret_cast<uint8_t*>(pointer) + adder); }
template<typename T>
const T* add_bytes_to_pointer(const T* pointer, ssize_t adder) { return reinterpret_cast<const T*>(reinterpret_cast<const uint8_t*>(pointer) + adder); }


template<typename T>
T unaligned_read(const void* ptr)
{
    struct Container { T value; } __attribute__ ((packed)); // Use a packed struct to prevent the compiler from making assumptions about alignment.
    return reinterpret_cast<const Container*>(ptr)->value;
}

template<typename T>
void unaligned_write(void* ptr, T value)
{
    struct Container { T value; } __attribute__((packed));
    reinterpret_cast<Container*>(ptr)->value = value;
}


template<typename T1, typename T2, typename T3> constexpr void set_bit_group(T1& target, T2 mask, T3 value) { target = (target & ~mask) | (value & mask); }

static constexpr uint32_t nanoseconds_to_cycles_floor(uint32_t clockFrequency, uint32_t nanoSeconds)
{
	return uint32_t((uint64_t(clockFrequency) * uint64_t(nanoSeconds)) / TimeValNanos::TicksPerSecond);
}

static constexpr uint32_t nanoseconds_to_cycles_ceil(uint32_t clockFrequency, uint32_t nanoSeconds)
{
	return uint32_t((uint64_t(clockFrequency) * uint64_t(nanoSeconds) + TimeValNanos::TicksPerSecond - 1) / TimeValNanos::TicksPerSecond);
}

static constexpr uint32_t time_to_cycles_floor(double clockFrequency, double timeValue)
{
    return uint32_t(std::floor(clockFrequency * timeValue));
}

static constexpr uint32_t time_to_cycles_ceil(double clockFrequency, double timeValue)
{
    return uint32_t(std::ceil(clockFrequency * timeValue));
}

namespace os
{
template<typename T>
struct ReverseRangedWrapper
{
    ReverseRangedWrapper(T& list) : m_List(list) {}
    
    auto begin() { return m_List.rbegin(); }
    auto end() { return m_List.rend(); }
    
    T& m_List;    
};

template<typename T>
struct ReverseRangedWrapperConst
{
    ReverseRangedWrapperConst(const T& list) : m_List(list) {}
    
    auto begin() { return m_List.rbegin(); }
    auto end() { return m_List.rend(); }
    
    const T& m_List;
};

template<typename T> ReverseRangedWrapper<T>      reverse_ranged(T& list) { return ReverseRangedWrapper<T>(list); }
template<typename T> ReverseRangedWrapperConst<T> reverse_ranged(const T& list) { return ReverseRangedWrapper<T>(list); }

template <typename O, typename F> auto bind_method(O* obj, F&& f) { return [=](auto&&... args) { return (obj->*f)(std::forward<decltype(args)>(args)...); }; }


class TimeoutTracker
{
public:
    TimeoutTracker(TimeValNanos timeout) : m_Deadline(get_monotonic_time() + timeout) {}
    TimeoutTracker(bigtime_t timeoutMilliseconds) : m_Deadline(get_monotonic_time() + TimeValNanos::FromMilliseconds(timeoutMilliseconds)) {}
    operator bool() const { return get_monotonic_time() < m_Deadline; }
private:
    TimeValNanos m_Deadline;
};

class ProfileTimer
{
    public:
        ProfileTimer(const String& title, TimeValNanos minimumTime = 0.0) : m_Title(title), m_MinimumTime(minimumTime) { m_StartTime = get_monotonic_time_hires(); m_PrevLapTime = m_StartTime; }
    ~ProfileTimer()
    {
        Terminate();
    }
    
    bool Terminate()
    {
        if (m_StartTime.AsNative() != 0)
        {
            const TimeValNanos curTime = get_monotonic_time_hires();
            const TimeValNanos time = curTime - m_StartTime;
            const TimeValNanos lapTime = curTime - m_PrevLapTime;
            m_StartTime = 0.0;
            if (time >= m_MinimumTime)
            {
                if (m_PrevLapTime == m_StartTime)
                {
                    p_system_log<PLogSeverity::INFO_LOW_VOL>(LogCat_General, "Prof: {} ({:.3f})", m_Title, time.AsSeconds() * 1000.0);
                }
                else
                {
                    p_system_log<PLogSeverity::INFO_LOW_VOL>(LogCat_General, "Prof: {} ({:.3f} / {:.3f})", m_Title, lapTime.AsSeconds() * 1000.0, time.AsSeconds() * 1000.0);
                }
                return true;
            }
        }
        return false;
    }
    bool Lap(const char* text)
    {
        if (m_StartTime.AsNative() != 0)
        {
            const TimeValNanos curTime = get_monotonic_time_hires();
            const TimeValNanos time = curTime - m_StartTime;
            if (time >= m_MinimumTime)
            {
                const TimeValNanos lapTime = curTime - m_PrevLapTime;
                m_PrevLapTime = curTime;
                p_system_log<PLogSeverity::INFO_LOW_VOL>(LogCat_General, "Prof: {} ({}) ({:.3f} / {:.3f})", m_Title, text, lapTime.AsSeconds() * 1000.0, time.AsSeconds() * 1000.0);
                return true;
            }
        }
        return false;
    }
    String    m_Title;
    TimeValNanos m_StartTime;
    TimeValNanos m_PrevLapTime;
    TimeValNanos m_MinimumTime;
};

} // namespace

#define I8(value) static_cast<int8_t>(value)
#define U8(value) static_cast<uint8_t>(value)

#define I16(value) static_cast<int16_t>(value)
#define U16(value) static_cast<uint16_t>(value)

#define I32(value) static_cast<int32_t>(value)
#define U32(value) static_cast<uint32_t>(value)

#define I64(value) static_cast<int64_t>(value)
#define U64(value) static_cast<uint64_t>(value)

#define BIT(pos, value) ((value)<<(pos))
#define BIT8(pos, value)  U8((value)<<(pos))
#define BIT16(pos, value) U16((value)<<(pos))
#define BIT32(pos, value) U32((value)<<(pos))

#define DIV_ROUND(x,divider) (((x) + ((divider) >> 1)) / (divider))

#define ARRAY_COUNT(a) (sizeof(a) / sizeof(a[0]))
