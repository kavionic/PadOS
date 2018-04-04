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
// Created: 08.03.2018 17:55:02

#pragma once

#include "System/Ptr/PtrTarget.h"
#include "System/System.h"
#include "Kernel.h"

namespace kernel
{

enum class KNamedObjectType
{
    Thread,
    Semaphore,
    MessagePort,
};

class KNamedObject : public PtrTarget
{
public:
    KNamedObject(const char* name, KNamedObjectType type);
    virtual ~KNamedObject();

    bool DebugValidate() const { if (m_Magic != MAGIC) { panic("KnamedObject has been overwritten!\n"); return false; } return true; }

    KNamedObjectType GetType() const           { return m_Type; }
    const char*      GetName() const           { return m_Name; }

    void             SetHandle(int32_t handle) { m_Handle = handle; }
    int32_t          GetHandle() const         { return m_Handle; }

    static int32_t           RegisterObject(Ptr<KNamedObject> object);
    static bool              FreeHandle(int32_t handle, KNamedObjectType type);

    static Ptr<KNamedObject> GetObject(int32_t handle, KNamedObjectType type);
    template<typename T>
    static Ptr<T>            GetObject(int32_t handle) { return ptr_static_cast<T>(GetObject(handle, T::ObjectType)); }

private:
    static const uint32_t MAGIC = 0x1ee3babe;
    uint32_t m_Magic = MAGIC;
    char    m_Name[OS_NAME_LENGTH];
    KNamedObjectType m_Type;
    int32_t m_Handle = -1;

    KNamedObject( const KNamedObject &c );
    KNamedObject& operator=( const KNamedObject &c );

};

} // namespace
