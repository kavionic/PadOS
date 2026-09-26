// This file is part of PadOS.
//
// Copyright (c) 2020-2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 11.07.2020 13:00

#pragma once

#include <sys/pados_syscalls.h>

#include <System/HandleObject.h>
#include <Threads/Threads.h>
#include <Threads/Mutex.h>


class PConditionVariable : public PHandleObject
{
public:
  enum class NoInit {};

  explicit PConditionVariable(NoInit) : PHandleObject(INVALID_HANDLE) {}
  PConditionVariable(const char* name = "", int clockID = CLOCK_MONOTONIC)
  {
      handle_id handle;
      if (condition_var_create(&handle, name, clockID) == PErrorCode::Success) {
          SetHandle(handle);
      }
  }
  ~PConditionVariable() { condition_var_delete(m_Handle); }

  bool Wait(PMutex& lock) { return ParseResult(condition_var_wait(m_Handle, lock.GetHandle())); }
  bool WaitTimeout(PMutex& lock, const TimeValNanos& timeout) { return ParseResult(condition_var_wait_timeout_ns(m_Handle, lock.GetHandle(), timeout.AsNanoseconds())); }
  bool WaitDeadline(PMutex& lock, const TimeValNanos& deadline) { return ParseResult(condition_var_wait_deadline_ns(m_Handle, lock.GetHandle(), deadline.AsNanoseconds())); }

  bool Wakeup(int threadCount) { return ParseResult(condition_var_wakeup(m_Handle, threadCount)); }
  bool WakeupAll() { return ParseResult(condition_var_wakeup_all(m_Handle)); }

  PConditionVariable(PConditionVariable&& other) = default;
  PConditionVariable(const PConditionVariable& other) = default;
  PConditionVariable& operator=(const PConditionVariable&) = default;

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
