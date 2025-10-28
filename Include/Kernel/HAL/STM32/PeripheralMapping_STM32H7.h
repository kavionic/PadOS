#pragma once

#include <stdint.h>
#include <Kernel/HAL/DigitalPort.h>

enum class DMAMUX_REQUEST : int;


enum class HWTimerID : int32_t
{
    None,
    Timer1,
    Timer2,
    Timer3,
    Timer4,
    Timer5,
    Timer6,
    Timer7,
    Timer8,
    unused9,
    unused10,
    unused11,
    Timer12,
    Timer13,
    Timer14,
    Timer15,
    Timer16,
    Timer17,
};

enum class HWTimerIRQType : int
{
    Break,
    Update,
    Trigger,
    Commutation,
    CaptureCompare
};

enum class USARTID : int
{
    None,
    LPUART_1,
    USART_1,
    USART_2,
    USART_3,
    UART_4,
    UART_5,
    USART_6,
    UART_7,
    UART_8
};

enum class I2CID : int
{
    None = 0,
    I2C_1 = 1,
    I2C_2 = 2,
    I2C_3 = 3,
    I2C_4 = 4
};

enum class I2CIRQType : int
{
    Event,
    Error
};

enum class SPIID : int
{
    None = 0,
    SPI_1 = 1,
    SPI_2 = 2,
    SPI_3 = 3,
    SPI_4 = 4,
    SPI_5 = 5,
    SPI_6 = 6
};

enum class USB_OTG_ID : int
{
    USB1_HS,
    USB2_FS
};

enum class ADC_ID : int
{
    ADC_1,
    ADC_2,
    ADC_3
};

PinMuxTarget pinmux_target_from_name(const char* name);
DigitalPinID pin_id_from_name(const char* name);

HWTimerID   timer_id_from_name(const char* name);
IRQn_Type   get_timer_irq(HWTimerID timerID, HWTimerIRQType irqType);

USARTID     usart_id_from_name(const char* name);
IRQn_Type   get_usart_irq(USARTID id);
bool        get_usart_dma_requests(USARTID id, DMAMUX_REQUEST& rx, DMAMUX_REQUEST& tx);

I2CID       i2c_id_from_name(const char* name);
IRQn_Type   get_i2c_irq(I2CID id, I2CIRQType type);

SPIID       spi_id_from_name(const char* name);
IRQn_Type   get_spi_irq(SPIID id);
bool        get_spi_dma_requests(SPIID id, DMAMUX_REQUEST& rx, DMAMUX_REQUEST& tx);

IRQn_Type   get_usb_irq(USB_OTG_ID id);
IRQn_Type   get_adc_irq(ADC_ID id);

namespace kernel
{

TIM_TypeDef* get_timer_from_id(HWTimerID timerID);
volatile uint32_t* get_timer_dbg_clk_flag(HWTimerID timerID, uint32_t& outFlagMask);

USART_TypeDef* get_usart_from_id(USARTID id);

I2C_TypeDef* get_i2c_from_id(I2CID id);

SPI_TypeDef* get_spi_from_id(SPIID id);

USB_OTG_GlobalTypeDef*  get_usb_from_id(USB_OTG_ID id);

ADC_TypeDef*        get_adc_from_id(ADC_ID id);
ADC_Common_TypeDef* get_adc_common_from_id(ADC_ID id);


} // namespace kernel

