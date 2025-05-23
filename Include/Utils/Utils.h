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
#include "System/Types.h"
#include "System/System.h"
#include "Utils/String.h"
#include "Kernel/Scheduler.h"

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

template<typename T>
constexpr T align_up(T value, T alignment) { return (value + alignment - 1) & ~(alignment - 1); }

template<typename T>
constexpr T align_down(T value, T alignment) { return value & ~(alignment - 1); }


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


template<typename T1, typename T2, typename T3> IFLASHC constexpr void set_bit_group(T1& target, T2 mask, T3 value) { target = (target & ~mask) | (value & mask); }

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
    TimeoutTracker(TimeValMicros timeout) : m_Deadline(get_system_time() + timeout) {}
    TimeoutTracker(bigtime_t timeoutMilliseconds) : m_Deadline(get_system_time() + TimeValMicros::FromMilliseconds(timeoutMilliseconds)) {}
    operator bool() const { return get_system_time() < m_Deadline; }
private:
    TimeValMicros m_Deadline;
};

class ProfileTimer
{
    public:
        ProfileTimer(const String& title, TimeValMicros minimumTime = 0.0) : m_Title(title), m_MinimumTime(minimumTime) { m_StartTime = get_system_time_hires(); m_PrevLapTime = m_StartTime; }
    ~ProfileTimer()
    {
        Terminate();
    }
    
    bool Terminate()
    {
        if (m_StartTime.AsNative() != 0)
        {
            const TimeValNanos curTime = get_system_time_hires();
            const TimeValNanos time = curTime - m_StartTime;
            const TimeValNanos lapTime = curTime - m_PrevLapTime;
            m_StartTime = 0.0;
            if (time >= m_MinimumTime)
            {
                if (m_PrevLapTime == m_StartTime)
                {
                    printf("Prof: %s (%.3f)\n", m_Title.c_str(), time.AsSeconds() * 1000.0);
                }
                else
                {
                    printf("Prof: %s (%.3f / %.3f)\n", m_Title.c_str(), lapTime.AsSeconds() * 1000.0, time.AsSeconds() * 1000.0);
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
            const TimeValNanos curTime = get_system_time_hires();
            const TimeValNanos time = curTime - m_StartTime;
            if (time >= m_MinimumTime)
            {
                const TimeValNanos lapTime = curTime - m_PrevLapTime;
                m_PrevLapTime = curTime;
                printf("Prof: %s (%s) (%.3f / %.3f)\n", m_Title.c_str(), text, lapTime.AsSeconds() * 1000.0, time.AsSeconds() * 1000.0);
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

struct DebugCallTracker
{
    DebugCallTracker(const char* currentFunction)
    {
        m_PrevTracker  = kernel::gk_CurrentThread->m_FirstDebugCallTracker;
        m_Function = currentFunction;
        kernel::gk_CurrentThread->m_FirstDebugCallTracker = this;
    }
    ~DebugCallTracker()
    {
        kernel::gk_CurrentThread->m_FirstDebugCallTracker  = m_PrevTracker;
    }
    
//    static const char*       s_CurrentFunction;
//    static DebugCallTracker* s_CurrentTracker;
    
    DebugCallTracker* m_PrevTracker;
//    const char*       m_PrevFunction;
    const char*       m_Function;
};

#define DEBUG_TRACK_FUNCTION() DebugCallTracker debugCallTracker__(__func__)

template<typename T> constexpr auto to_underlying(T e) { return static_cast<std::underlying_type_t<T>>(e); }

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

#ifndef PIN0_bp
#define PIN0_bp 0
#define PIN1_bp 1
#define PIN2_bp 2
#define PIN3_bp 3
#define PIN4_bp 4
#define PIN5_bp 5
#define PIN6_bp 6
#define PIN7_bp 7

#define PIN8_bp 8
#define PIN9_bp 9
#define PIN10_bp 10
#define PIN11_bp 11
#define PIN12_bp 12
#define PIN13_bp 13
#define PIN14_bp 14
#define PIN15_bp 15

#define PIN16_bp 16
#define PIN17_bp 17
#define PIN18_bp 18
#define PIN19_bp 19
#define PIN20_bp 20
#define PIN21_bp 21
#define PIN22_bp 22
#define PIN23_bp 23

#define PIN24_bp 24
#define PIN25_bp 25
#define PIN26_bp 26
#define PIN27_bp 27
#define PIN28_bp 28
#define PIN29_bp 29
#define PIN30_bp 30
#define PIN31_bp 31
#endif // PIN0_bp

#ifndef PIN0_bm
#define PIN0_bm BIT8(PIN0_bp, 1)
#define PIN1_bm BIT8(PIN1_bp, 1)
#define PIN2_bm BIT8(PIN2_bp, 1)
#define PIN3_bm BIT8(PIN3_bp, 1)
#define PIN4_bm BIT8(PIN4_bp, 1)
#define PIN5_bm BIT8(PIN5_bp, 1)
#define PIN6_bm BIT8(PIN6_bp, 1)
#define PIN7_bm BIT8(PIN7_bp, 1)

#define PIN8_bm  BIT16(PIN8_bp, 1)
#define PIN9_bm  BIT16(PIN9_bp, 1)
#define PIN10_bm BIT16(PIN10_bp, 1)
#define PIN11_bm BIT16(PIN11_bp, 1)
#define PIN12_bm BIT16(PIN12_bp, 1)
#define PIN13_bm BIT16(PIN13_bp, 1)
#define PIN14_bm BIT16(PIN14_bp, 1)
#define PIN15_bm BIT16(PIN15_bp, 1)

#define PIN16_bm BIT32(PIN16_bp, 1)
#define PIN17_bm BIT32(PIN17_bp, 1)
#define PIN18_bm BIT32(PIN18_bp, 1)
#define PIN19_bm BIT32(PIN19_bp, 1)
#define PIN20_bm BIT32(PIN20_bp, 1)
#define PIN21_bm BIT32(PIN21_bp, 1)
#define PIN22_bm BIT32(PIN22_bp, 1)
#define PIN23_bm BIT32(PIN23_bp, 1)

#define PIN24_bm BIT32(PIN24_bp, 1)
#define PIN25_bm BIT32(PIN25_bp, 1)
#define PIN26_bm BIT32(PIN26_bp, 1)
#define PIN27_bm BIT32(PIN27_bp, 1)
#define PIN28_bm BIT32(PIN28_bp, 1)
#define PIN29_bm BIT32(PIN29_bp, 1)
#define PIN30_bm BIT32(PIN30_bp, 1)
#define PIN31_bm BIT32(PIN31_bp, 1)
#endif // PIN0_bm

#define EXPECT_TRUE(expr) if (!(expr)) { printf("TEST FAILED: %s\n", #expr);}
#define EXPECT_FALSE(expr) if ((expr)) { printf("TEST FAILED: %s\n", #expr);}
#define EXPECT_NEAR(expr1, expr2, abs_error) if (fabs((expr1) - (expr2)) > abs_error) { printf("TEST FAILED: %s != %s\n", #expr1, #expr2); }
