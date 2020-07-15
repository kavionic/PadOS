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
#include "Types.h"

namespace os
{


class HandleObject
{
public:
  enum class NoInit {};
  
  HandleObject(handler_id handle) : m_Handle(handle) {}
  virtual ~HandleObject();

  handle_id GetHandle() const { return m_Handle; }

  HandleObject(HandleObject&& other) : m_Handle(other.m_Handle) { other.m_Handle = INVALID_HANDLE; }

  HandleObject(const HandleObject& other);
  HandleObject& operator=(const HandleObject& other);

protected:
  handle_id m_Handle;
};

} // namespace
