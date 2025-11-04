// This file is part of PadOS.
//
// Copyright (C) 2020-2025 Kurt Skauen <http://kavionic.com/>
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
// Created: 11.07.2020 13:00

#pragma once

#include <sys/pados_syscalls.h>

#include <System/HandleObject.h>
#include <Threads/Threads.h>
#include <Threads/Mutex.h>

namespace os
{

class ConditionVariable : public HandleObject
{
public:
  enum class NoInit {};

  explicit ConditionVariable(NoInit) : HandleObject(INVALID_HANDLE) {}
  ConditionVariable(const char* name = "", int clockID = CLOCK_MONOTONIC)
  {
      handle_id handle;
      if (condition_var_create(&handle, name, clockID) == PErrorCode::Success) {
          SetHandle(handle);
      }
  }
  ~ConditionVariable() { condition_var_delete(m_Handle); }

  bool Wait(Mutex& lock) { return ParseResult(condition_var_wait(m_Handle, lock.GetHandle())); }
  bool WaitTimeout(Mutex& lock, const TimeValNanos& timeout) { return ParseResult(condition_var_wait_timeout_ns(m_Handle, lock.GetHandle(), timeout.AsNanoseconds())); }
  bool WaitDeadline(Mutex& lock, const TimeValNanos& deadline) { return ParseResult(condition_var_wait_deadline_ns(m_Handle, lock.GetHandle(), deadline.AsNanoseconds())); }

  bool Wakeup(int threadCount) { return ParseResult(condition_var_wakeup(m_Handle, threadCount)); }
  bool WakeupAll() { return ParseResult(condition_var_wakeup_all(m_Handle)); }

  ConditionVariable(ConditionVariable&& other) = default;
  ConditionVariable(const ConditionVariable& other) = default;
  ConditionVariable& operator=(const ConditionVariable&) = default;

private:
    bool ParseResult(PErrorCode result) const
    {
        if (result == PErrorCode::Success)
        {
            return true;
        }
        else
        {
            set_last_error(result);
            return false;
        }
    }
};


} // namespace

using PConditionVariable = os::ConditionVariable;
