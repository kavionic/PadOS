#ifdef STM32G0

#include <string.h>
#include <Kernel/HAL/STM32/PeripheralMapping_STM32G030xx.h>

namespace kernel
{

HWTimerID timer_id_from_name(const char* name)
{
    if (name[0] != 'T' || name[1] != 'I' || name[2] != 'M')
    {
        p_system_log<PLogSeverity::ERROR>(LogCatKernel_General, "timer_id_from_name() failed to convert '{}'", name);
        return HWTimerID::None;
    }
    return HWTimerID(atoi(name + 3));
}

TIM_TypeDef* get_timer_from_id(HWTimerID timerID)
{
    switch (timerID)
    {
        case HWTimerID::Timer1:     return TIM1;
        case HWTimerID::Timer3:     return TIM3;
        case HWTimerID::Timer14:    return TIM14;
        case HWTimerID::Timer16:    return TIM16;
        case HWTimerID::Timer17:    return TIM17;
        default:
            p_system_log<PLogSeverity::ERROR>(LogCatKernel_General, "get_timer_from_id() unknown timer '{}'", int(timerID));
            return nullptr;
    }
}

IRQn_Type get_timer_irq(HWTimerID timerID, HWTimerIRQType irqType)
{
    switch (timerID)
    {
        case HWTimerID::Timer1:
            switch (irqType)
            {
                case HWTimerIRQType::Break:             return TIM1_BRK_UP_TRG_COM_IRQn;
                case HWTimerIRQType::Update:            return TIM1_BRK_UP_TRG_COM_IRQn;
                case HWTimerIRQType::Trigger:           return TIM1_BRK_UP_TRG_COM_IRQn;
                case HWTimerIRQType::Commutation:       return TIM1_BRK_UP_TRG_COM_IRQn;
                case HWTimerIRQType::CaptureCompare:    return TIM1_CC_IRQn;
                default:    return IRQn_Type(IRQ_COUNT);
            }
        case HWTimerID::Timer3:     return TIM3_IRQn;
        case HWTimerID::Timer14:    return TIM14_IRQn;
        case HWTimerID::Timer16:    return TIM16_IRQn;
        case HWTimerID::Timer17:    return TIM17_IRQn;
        default:
            p_system_log<PLogSeverity::ERROR>(LogCatKernel_General, "get_timer_irq() unknown timer '{}'", int(timerID));
            return IRQn_Type(IRQ_COUNT);
    }
}

I2CID i2c_id_from_name(const char* name)
{
    if (strncmp(name, "I2C", 3) == 0 && name[3] >= '1' && name[3] <= '2') {
        return I2CID(int(name[3] - '0'));
    } else {
        return I2CID::None;
    }
}

I2C_TypeDef* get_i2c_from_id(I2CID id)
{
    switch (id)
    {
        case I2CID::I2C_1:  return I2C1;
        case I2CID::I2C_2:  return I2C2;
        default: return nullptr;
    }
}


IRQn_Type get_i2c_irq(I2CID id)
{
    switch (id)
    {
        case I2CID::I2C_1:  return I2C1_IRQn;
        case I2CID::I2C_2:  return I2C2_IRQn;
        default: return IRQn_Type(IRQ_COUNT);
    }
}


SPIID spi_id_from_name(const char* name)
{
    if (strncmp(name, "SPI", 3) == 0 && name[3] >= '1' && name[3] <= '6') {
        return SPIID(int(name[3] - '0'));
    }
    else {
        return SPIID::None;
    }
}

SPI_TypeDef* get_spi_from_id(SPIID id)
{
    switch (id)
    {
        case SPIID::SPI_1:  return SPI1;
        case SPIID::SPI_2:  return SPI2;
        default: return nullptr;
    }
}


IRQn_Type get_spi_irq(SPIID id)
{
    switch (id)
    {
        case SPIID::SPI_1:  return SPI1_IRQn;
        case SPIID::SPI_2:  return SPI2_IRQn;
        default: return IRQn_Type(IRQ_COUNT);
    }
}

//bool get_spi_dma_requests(SPIID id, DMAMUX_REQUEST& rx, DMAMUX_REQUEST& tx)
//{
//    switch (id)
//    {
//        case SPIID::SPI_1:
//            rx = DMAMUX_REQUEST::REQ_SPI1_RX;
//            tx = DMAMUX_REQUEST::REQ_SPI1_TX;
//            return true;
//        case SPIID::SPI_2:
//            rx = DMAMUX_REQUEST::REQ_SPI2_RX;
//            tx = DMAMUX_REQUEST::REQ_SPI2_TX;
//            return true;
//        default:    return false;
//    }
//}

} // namespace kernel

#endif // STM32G0
