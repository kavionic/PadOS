// This file is part of PadOS.
//
// Copyright (C) 2025 Kurt Skauen <http://kavionic.com/>
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
// Created: 25.11.2025 01:00

#pragma once

#include <stdint.h>

#include <PadOS/Threads.h>
#include <System/ModuleTLSDefinition.h>

struct PAppDefinition
{
    void (*Entry)();
    void (*ThreadTerminated)(thread_id, void*, PThreadControlBlock*);
    
    PModuleTLSDefinition TLSDefinition;
};

extern "C"
{

extern PAppDefinition __kernel_definition;
extern PAppDefinition __app_definition;
extern PThreadControlBlock* __app_thread_data;

} // extern "C"

