#include <Kernel/HAL/DigitalPort.h>


DigitalPort::DigitalPort(DigitalPortID port) : m_Port(port)
{

}

IFLASHC void DigitalPort::SetPeripheralMux(DigitalPortID portID, PortData_t pins, DigitalPinPeripheralID peripheral)
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

IFLASHC void DigitalPort::SetDirection(DigitalPortID portID, DigitalPinDirection_e direction, PortData_t pins)
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

IFLASHC void DigitalPort::SetDriveStrength(DigitalPortID portID, DigitalPinDriveStrength_e strength, PortData_t pins)
{
    GPIO_Port_t* port = GetPortRegs(portID);
    for (uint32_t i = 0, j = 0x0001; j != 0; j <<= 1, i += 2) {
        if (j & pins) {
            port->OSPEEDR = (port->OSPEEDR & ~(3 << i)) | (uint32_t(strength) << i);
        }
    }
}

IFLASHC void DigitalPort::SetPullMode(DigitalPortID portID, PinPullMode_e mode, PortData_t pins)
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

void DigitalPin::ActivatePeripheralMux(const PinMuxTarget& PinMux)
{
    DigitalPin(PinMux.PINID).SetPeripheralMux(PinMux.MUX);
}

IFLASHC void DigitalPin::Write(bool value)
{
    if (value) m_Port.SetHigh(m_PinMask); else m_Port.SetLow(m_PinMask);
}

IFLASHC bool DigitalPin::Read() const
{
    return (m_Port.Get() & m_PinMask) != 0;
}
