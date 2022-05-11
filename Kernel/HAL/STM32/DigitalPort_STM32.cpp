#include <Kernel/HAL/DigitalPort.h>

void DigitalPin::ActivatePeripheralMux(const PinMuxTarget& PinMux)
{
    DigitalPin(PinMux.PINID).SetPeripheralMux(PinMux.MUX);
}
