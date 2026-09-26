// This file is part of PadOS.
//
// Copyright (c) 2018 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 11.03.2018 16:01:50

#include "Threads/EventHandler.h"
#include "Signals/RemoteSignal.h"


PEventHandler::PEventHandler(const PString& name) : m_Name(name)
{
    static handler_id nextHandle = 0;
    m_Handle = ++nextHandle;
}

PEventHandler::~PEventHandler()
{
}

bool PEventHandler::HandleMessage(int32_t code, const void* data, size_t length)
{
    return m_RemoteSignalRegistry.Dispatch(code, data, length);
}
