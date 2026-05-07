// This file is part of PadOS.
//
// Copyright (C) 2026 Kurt Skauen <http://kavionic.com/>
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
// Created: 21.02.2026 22:00

#pragma once

#include <atomic>
#include <functional>
#include <vector>

#include <Kernel/KMutex.h>
#include <Kernel/KThread.h>
#include <Math/Point.h>
#include <Utils/ANSIEscapeCodeParser.h>

namespace kernel
{


class KSerialPseudoTerminal : public KThread
{
public:
    KSerialPseudoTerminal();

    void Setup();
    void SetSerialWriteCallback(std::function<void(const char*, size_t)> callback);

    virtual void* Run() override;

    void ReceiveData(const char* buffer, size_t length);
    void ProcessSerialInput(const char* buffer, size_t length);

    void SendToSerial(const char* text, size_t length);
    void SendToSerial(const PString& text) { SendToSerial(text.c_str(), text.size()); }
    void SendToSlave(const char* text, size_t length) { write(m_MasterPTY, text, length); }
    void SendToSlave(const PString& text) { SendToSlave(text.c_str(), text.size()); }

    template<typename... TArgTypes>
    void SendANSICode(PANSI_ControlCode code, TArgTypes ...args)
    {
        SendToSerial(m_ANSICodeParser.FormatANSICode(code, std::forward<TArgTypes>(args)...));
    }
    void ProcessControlChar(PANSI_ControlCode controlChar, const std::vector<int>& args);

private:
    std::function<void(const char*, size_t)> m_OnSendToSerial;

    int m_MasterPTY = -1;
    int m_SlavePTY  = -1;
    std::atomic<bool> m_PTYReady{false};

    KMutex            m_IncomingMutex;
    std::vector<char> m_IncomingData;

    PANSIEscapeCodeParser m_ANSICodeParser;
    PIPoint               m_TerminalSize = PIPoint(80, 24);
};


} // namespace kernel
