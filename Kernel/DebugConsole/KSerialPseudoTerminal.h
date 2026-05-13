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
#include <vector>

#include <Kernel/KSemaphore.h>
#include <Kernel/KThread.h>
#include <Math/Point.h>
#include <Utils/ANSIEscapeCodeParser.h>

namespace kernel
{


class KSerialPseudoTerminal : public KThread
{
public:
    KSerialPseudoTerminal(int serialReadFD, int serialWriteFD, bool uartMode = false);

    void Setup();

    virtual void* Run() override;

    void ProcessSerialInput(const char* buffer, size_t length);
    void SetTerminalSize(uint16_t width, uint16_t height, uint16_t pixelWidth, uint16_t pixelHeight);

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
    void ApplyTerminalSize(uint16_t width, uint16_t height, uint16_t pixelWidth, uint16_t pixelHeight);

    int  m_SerialReadFD      = -1;
    int  m_SerialWriteFD     = -1;
    bool m_UARTMode = false;

    KSemaphore m_TerminalSizeNotifier;

    int m_MasterPTY = -1;

    std::atomic<bool> m_PTYReady{false};

    std::atomic<bool>     m_PendingTerminalSizeChange{false};
    std::atomic<uint16_t> m_PendingWidth{80};
    std::atomic<uint16_t> m_PendingHeight{24};
    std::atomic<uint16_t> m_PendingPixelWidth{0};
    std::atomic<uint16_t> m_PendingPixelHeight{0};

    PANSIEscapeCodeParser m_ANSICodeParser;
    PIPoint               m_TerminalSize = PIPoint(80, 24);
};


} // namespace kernel
