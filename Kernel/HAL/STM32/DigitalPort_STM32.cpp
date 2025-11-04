#include <Kernel/HAL/DigitalPort.h>
#include <Utils/Utils.h>


DigitalPort::DigitalPort(DigitalPortID port) : m_Port(port)
{

}

GPIO_Port_t* DigitalPort::GetPortRegs(DigitalPortID portID)
{
    return DigitalPortsRegisters[portID];
}

void DigitalPort::SetPeripheralMux(DigitalPortID portID, PortData_t pins, DigitalPinPeripheralID peripheral)
{
    GPIO_Port_t* port = GetPortRegs(portID);
    if (peripheral == DigitalPinPeripheralID::None)
    {
        for (uint32_t i = 0, j = 0x0001; j != 0; j <<= 1, i += 2)
        {
            if (j & pins) {
                port->MODER = (port->MODER & ~(3 << i)) | (3 << i); // Mode 3: analog (reset state)
            }
        }
    }
    else
    {
        uint32_t value = uint32_t(peripheral) & 0xf;
        uint32_t valueMask = 0xf;

        int modeBitPos = 0;
        for (uint32_t pinsL = pins & 0xff; pinsL != 0; pinsL >>= 1, value <<= 4, valueMask <<= 4)
        {
            if (pinsL & 0x01)
            {
                set_bit_group(port->AFR[0], valueMask, value);
                set_bit_group(port->MODER, 3 << modeBitPos, 2 << modeBitPos); // Mode 2: alternate function
            }
            modeBitPos += 2;
        }
        modeBitPos = 2 * 8;
        value = uint32_t(peripheral) & 0xf;
        valueMask = 0xf;
        for (uint32_t pinsH = (pins >> 8) & 0xff; pinsH != 0; pinsH >>= 1, value <<= 4, valueMask <<= 4)
        {
            if (pinsH & 0x01)
            {
                set_bit_group(port->AFR[1], valueMask, value);
                set_bit_group(port->MODER, 3 << modeBitPos, 2 << modeBitPos); // Mode 2: alternate function
            }
            modeBitPos += 2;
        }
    }
}

void DigitalPort::SetPeripheralMux(PortData_t pins, DigitalPinPeripheralID peripheral)
{
    SetPeripheralMux(m_Port, pins, peripheral);
}

void DigitalPort::SetDirection(DigitalPortID portID, DigitalPinDirection_e direction, PortData_t pins)
{
    GPIO_Port_t* port = GetPortRegs(portID);
    switch (direction)
    {
        case DigitalPinDirection_e::Analog:
            for (uint32_t i = 0, j = 0x0001; j != 0; j <<= 1, i += 2)
            {
                if (j & pins) {
                    port->MODER = port->MODER | (3 << i); // Mode 3: analog
                }
            }
            break;
        case DigitalPinDirection_e::In:
            for (uint32_t i = 0, j = 0x0001; j != 0; j <<= 1, i += 2)
            {
                if (j & pins) {
                    port->MODER = (port->MODER & ~(3 << i)); // Mode 0: input
                }
            }
            break;
        case DigitalPinDirection_e::Out:
            for (uint32_t i = 0, j = 0x0001; j != 0; j <<= 1, i += 2)
            {
                if (j & pins) {
                    port->MODER = (port->MODER & ~(3 << i)) | (1 << i); // Mode 1: output
                }
            }
            port->OTYPER &= ~pins; // 0: push-pull
            break;
        case DigitalPinDirection_e::OpenCollector:
            for (uint32_t i = 0, j = 0x0001; j != 0; j <<= 1, i += 2)
            {
                if (j & pins) {
                    port->MODER = (port->MODER & ~(3 << i)) | (1 << i); // Mode 1: output
                }
            }
            port->OTYPER |= pins; // 1: open collector
            break;
    }
}

void DigitalPort::SetDirection(DigitalPinDirection_e direction, PortData_t pins)
{
    SetDirection(m_Port, direction, pins);
}

void DigitalPort::SetDriveStrength(DigitalPortID portID, DigitalPinDriveStrength_e strength, PortData_t pins)
{
    GPIO_Port_t* port = GetPortRegs(portID);
    for (uint32_t i = 0, j = 0x0001; j != 0; j <<= 1, i += 2) {
        if (j & pins) {
            port->OSPEEDR = (port->OSPEEDR & ~(3 << i)) | (uint32_t(strength) << i);
        }
    }
}

void DigitalPort::SetDriveStrength(DigitalPinDriveStrength_e strength, PortData_t pins)
{
    SetDriveStrength(m_Port, strength, pins);
}

void DigitalPort::SetPullMode(DigitalPortID portID, PinPullMode_e mode, PortData_t pins)
{
    GPIO_Port_t* port = GetPortRegs(portID);
    switch (mode)
    {
        case PinPullMode_e::None:
            for (uint32_t i = 0, j = 0x0001; j != 0; j <<= 1, i += 2) {
                if (j & pins) {
                    port->PUPDR = (port->PUPDR & ~(3 << i)) | (0 << i); // 0: no pull
                }
            }
            break;
        case PinPullMode_e::Down:
            for (uint32_t i = 0, j = 0x0001; j != 0; j <<= 1, i += 2) {
                if (j & pins) {
                    port->PUPDR = (port->PUPDR & ~(3 << i)) | (2 << i); // 2: pull down
                }
            }
            break;
        case PinPullMode_e::Up:
            for (uint32_t i = 0, j = 0x0001; j != 0; j <<= 1, i += 2) {
                if (j & pins) {
                    port->PUPDR = (port->PUPDR & ~(3 << i)) | (1 << i); // 1: pull up
                }
            }
            break;
    }
}

void DigitalPort::SetPullMode(PinPullMode_e mode, PortData_t pins)
{
    SetPullMode(m_Port, mode, pins);
}

void DigitalPort::Set(PortData_t pins)
{
    Set(m_Port, pins);
}

void DigitalPort::Set(DigitalPortID portID, PortData_t pins)
{
    DigitalPortsRegisters[portID]->ODR = pins;
}

void DigitalPort::SetHigh(DigitalPortID portID, PortData_t pins)
{
    DigitalPortsRegisters[portID]->BSRR = pins;
}

void DigitalPort::SetHigh(PortData_t pins)
{
    SetHigh(m_Port, pins);
}

void DigitalPort::SetLow(DigitalPortID portID, PortData_t pins)
{
    DigitalPortsRegisters[portID]->BSRR = pins << 16;
}

void DigitalPort::SetLow(PortData_t pins)
{
    SetLow(m_Port, pins);
}

DigitalPort::PortData_t DigitalPort::Get(DigitalPortID portID)
{
    return DigitalPortsRegisters[portID]->IDR;
}

DigitalPort::PortData_t DigitalPort::Get() const
{
    return Get(m_Port);
}

DigitalPin::DigitalPin(DigitalPortID port, int pin) : m_PinID(DigitalPinID(MAKE_DIGITAL_PIN_ID(port, pin))), m_Port(port), m_PinMask(BIT32(pin, 1))
{

}

DigitalPin::DigitalPin(DigitalPinID pinID) : m_PinID(pinID), m_Port(DIGITAL_PIN_ID_PORT(pinID)), m_PinMask((pinID != DigitalPinID::None) ? BIT32(DIGITAL_PIN_ID_PIN(pinID), 1) : 0)
{

}

DigitalPin::DigitalPin() : m_PinID(DigitalPinID::None), m_Port(e_DigitalPortID_None), m_PinMask(0)
{

}

void DigitalPin::Set(DigitalPinID pinID)
{
    m_PinID = pinID;
    m_Port = DIGITAL_PIN_ID_PORT(pinID);
    m_PinMask = (pinID != DigitalPinID::None) ? BIT32(DIGITAL_PIN_ID_PIN(pinID), 1) : 0;
}

void DigitalPin::Set(DigitalPortID port, int pin)
{
    m_Port = port; m_PinMask = BIT32(pin, 1);
}

DigitalPinID DigitalPin::GetID() const
{
    return m_PinID;
}

bool DigitalPin::IsValid() const
{
    return m_PinID != DigitalPinID::None;
}

void DigitalPin::SetDirection(DigitalPinDirection_e dir)
{
    m_Port.SetDirection(dir, m_PinMask);
}

void DigitalPin::SetDriveStrength(DigitalPinDriveStrength_e strength)
{
    m_Port.SetDriveStrength(strength, m_PinMask);
}

void DigitalPin::SetPullMode(PinPullMode_e mode)
{
    m_Port.SetPullMode(mode, m_PinMask);
}

void DigitalPin::SetPeripheralMux(DigitalPinPeripheralID peripheral)
{
    m_Port.SetPeripheralMux(m_PinMask, peripheral);
}

void DigitalPin::ActivatePeripheralMux(const PinMuxTarget& PinMux)
{
    DigitalPin(PinMux.PINID).SetPeripheralMux(PinMux.MUX);
}

void DigitalPin::EnableInterrupts()
{
    EXTI->IMR1 |= m_PinMask;
}

void DigitalPin::DisableInterrupts()
{
    EXTI->IMR1 &= ~m_PinMask;
}

void DigitalPin::SetInterruptMode(PinInterruptMode_e mode)
{
    int pinIndex = DIGITAL_PIN_ID_PIN(m_PinID);

    if (mode == PinInterruptMode_e::FallingEdge || mode == PinInterruptMode_e::BothEdges) {
        EXTI->FTSR1 |= m_PinMask; // EXTI1 falling edge enabled.
    }
    else {
        EXTI->FTSR1 &= ~m_PinMask; // EXTI1 falling edge enabled.
    }
    if (mode == PinInterruptMode_e::RisingEdge || mode == PinInterruptMode_e::BothEdges) {
        EXTI->RTSR1 |= m_PinMask; // EXTI1 rising edge enabled.
    }
    else {
        EXTI->RTSR1 &= ~m_PinMask; // EXTI1 rising edge enabled.
    }
    if (mode != PinInterruptMode_e::None)
    {
        int portIndex = DIGITAL_PIN_ID_PORT(m_PinID);
        uint32_t regIndex = pinIndex >> 2;
        uint32_t groupPos = (pinIndex & 0x03) * 4;
        uint32_t mask = 0x000f << groupPos;
#if defined(STM32H7)
        SYSCFG->EXTICR[regIndex] = (SYSCFG->EXTICR[regIndex] & ~mask) | (portIndex << groupPos); // Route signals from this port to EXTI.
#elif defined(STM32G0)
        EXTI->EXTICR[regIndex] = (EXTI->EXTICR[regIndex] & ~mask) | (portIndex << groupPos); // Route signals from this port to EXTI.
#else
#error Unknown platform.
#endif
    }
}

void DigitalPin::Write(bool value)
{
    if (value) m_Port.SetHigh(m_PinMask); else m_Port.SetLow(m_PinMask);
}

bool DigitalPin::Read() const
{
    return (m_Port.Get() & m_PinMask) != 0;
}

DigitalPin::operator bool()
{
    return Read();
}

DigitalPin& DigitalPin::operator=(bool value)
{
    Write(value); return *this;
}

DigitalPin& DigitalPin::operator=(DigitalPinID pinID)
{
    Set(pinID); return *this;
}
