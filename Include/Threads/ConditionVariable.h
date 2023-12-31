// This file is part of PadOS.
//
// Copyright (C) 2020 Kurt Skauen <http://kavionic.com/>
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

#include "System/HandleObject.h"
#include "Threads.h"
#include "Mutex.h"

namespace os
{

class ConditionVariable : public HandleObject
{
public:
  enum class NoInit {};

  explicit ConditionVariable(NoInit) : HandleObject(INVALID_HANDLE) {}
  ConditionVariable(const char* name = "") : HandleObject(create_condition_var(name)) {}

  bool Wait(Mutex& lock) { return condition_var_wait(m_Handle, lock.GetHandle()) >= 0; }
  bool WaitTimeout(Mutex& lock, const TimeValMicros& timeout) { return condition_var_wait_timeout(m_Handle, lock.GetHandle(), timeout.AsMicroSeconds()) >= 0; }
  bool WaitDeadline(Mutex& lock, const TimeValMicros& deadline) { return condition_var_wait_deadline(m_Handle, lock.GetHandle(), deadline.AsMicroSeconds()) >= 0; }

  void Wakeup(int threadCount) { condition_var_wakeup(m_Handle, threadCount); }
  void WakeupAll() { condition_var_wakeup_all(m_Handle); }

  ConditionVariable(ConditionVariable&& other) = default;
  ConditionVariable(const ConditionVariable& other) = default;
  ConditionVariable& operator=(const ConditionVariable&) = default;

private:
};


} // namespace
