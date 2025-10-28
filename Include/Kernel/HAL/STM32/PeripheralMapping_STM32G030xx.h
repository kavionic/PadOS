#pragma once

#include <stdint.h>
#include <Kernel/HAL/DigitalPort.h>

enum class DMAMUX_REQUEST : int;

enum class HWTimerID : int32_t
{
    None,
    Timer1,
    unused2,
    Timer3,
    unused4,
    unused5,
    unused6,
    unused7,
    unused8,
    unused9,
    unused10,
    unused11,
    unused12,
    unused13,
    Timer14,
    unused15,
    Timer16,
    Timer17
};

enum class HWTimerIRQType : int
{
    Break,
    Update,
    Trigger,
    Commutation,
    CaptureCompare
};

enum class I2CID : int
{
    None = 0,
    I2C_1 = 1,
    I2C_2 = 2
};


enum class SPIID : int
{
    None = 0,
    SPI_1 = 1,
    SPI_2 = 2
};

namespace kernel
{

HWTimerID timer_id_from_name(const char* name);
TIM_TypeDef* get_timer_from_id(HWTimerID timerID);
IRQn_Type get_timer_irq(HWTimerID timerID, HWTimerIRQType irqType);

//USARTID usart_id_from_name(const char* name);
//USART_TypeDef* get_usart_from_id(USARTID id);
//IRQn_Type get_usart_irq(USARTID id);
//bool get_usart_dma_requests(USARTID id, DMAMUX_REQUEST& rx, DMAMUX_REQUEST& tx);

I2CID i2c_id_from_name(const char* name);
I2C_TypeDef* get_i2c_from_id(I2CID id);
IRQn_Type get_i2c_irq(I2CID id);

SPIID spi_id_from_name(const char* name);
SPI_TypeDef* get_spi_from_id(SPIID id);
IRQn_Type get_spi_irq(SPIID id);
//bool get_spi_dma_requests(SPIID id, DMAMUX_REQUEST& rx, DMAMUX_REQUEST& tx);

} // namespace kernel
