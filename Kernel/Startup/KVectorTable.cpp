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
// Created: 25.11.2025 00:30


#include <stdint.h>

extern "C"
{

void Reset_Handler(void);
void NonMaskableInt_Handler(void);
void HardFault_Handler(void);
void MemoryManagement_Handler(void);
void BusFault_Handler(void);
void UsageFault_Handler(void);
void SVCall_Handler(void);
void DebugMonitor_Handler(void);
void PendSV_Handler(void);
void SysTick_Handler(void);
void KernelHandleIRQ(void);

extern uint32_t _estack;

typedef struct _DeviceVectors
{
    /* Stack pointer */
    void* pvStack;
    /* Cortex-M handlers */
    void* pfnReset_Handler;                 // -15 Reset Vector, invoked on Power up and warm reset
    void* pfnNonMaskableInt_Handler;        // -14 Non maskable Interrupt, cannot be stopped or preempted
    void* pfnHardFault_Handler;             // -13 Hard Fault, all classes of Fault   
    void* pfnMemoryManagement_Handler;      // -12 Memory Management, MPU mismatch, including Access Violation and No Match
    void* pfnBusFault_Handler;              // -11 Bus Fault, Pre-Fetch-, Memory Access Fault, other address/memory related Fault
    void* pfnUsageFault_Handler;            // -10 Usage Fault, i.e. Undef Instruction, Illegal State Transition
    void* pvReservedC9;
    void* pvReservedC8;
    void* pvReservedC7;
    void* pvReservedC6;
    void* pfnSVCall_Handler;                //  -5 System Service Call via SVC instruction
    void* pfnDebugMonitor_Handler;          //  -4 Debug Monitor
    void* pvReservedC3;
    void* pfnPendSV_Handler;                //  -2 Pendable request for system service
    void* pfnSysTick_Handler;               //  -1 System Tick Timer

    void* pfnWWDG_IRQHandler;               // Window WatchDog              
    void* pfnPVD_AVD_IRQHandler;            // PVD/AVD through EXTI Line detection 
    void* pfnTAMP_STAMP_IRQHandler;         // Tamper and TimeStamps through the EXTI line 
    void* pfnRTC_WKUP_IRQHandler;           // RTC Wakeup through the EXTI line 
    void* pfnFLASH_IRQHandler;              // FLASH                        
    void* pfnRCC_IRQHandler;                // RCC                          
    void* pfnEXTI0_IRQHandler;              // EXTI Line0                   
    void* pfnEXTI1_IRQHandler;              // EXTI Line1                   
    void* pfnEXTI2_IRQHandler;              // EXTI Line2                   
    void* pfnEXTI3_IRQHandler;              // EXTI Line3                   
    void* pfnEXTI4_IRQHandler;              // EXTI Line4                   
    void* pfnDMA1_Stream0_IRQHandler;       // DMA1 Stream 0                
    void* pfnDMA1_Stream1_IRQHandler;       // DMA1 Stream 1                
    void* pfnDMA1_Stream2_IRQHandler;       // DMA1 Stream 2                
    void* pfnDMA1_Stream3_IRQHandler;       // DMA1 Stream 3                
    void* pfnDMA1_Stream4_IRQHandler;       // DMA1 Stream 4                
    void* pfnDMA1_Stream5_IRQHandler;       // DMA1 Stream 5                
    void* pfnDMA1_Stream6_IRQHandler;       // DMA1 Stream 6                
    void* pfnADC_IRQHandler;                // ADC1, ADC2 and ADC3s         
    void* pfnFDCAN1_IT0_IRQHandler;         // FDCAN1 interrupt line 0      
    void* pfnFDCAN2_IT0_IRQHandler;         // FDCAN2 interrupt line 0      
    void* pfnFDCAN1_IT1_IRQHandler;         // FDCAN1 interrupt line 1      
    void* pfnFDCAN2_IT1_IRQHandler;         // FDCAN2 interrupt line 1      
    void* pfnEXTI9_5_IRQHandler;            // External Line[9:5]s          
    void* pfnTIM1_BRK_IRQHandler;           // TIM1 Break interrupt         
    void* pfnTIM1_UP_IRQHandler;            // TIM1 Update interrupt        
    void* pfnTIM1_TRG_COM_IRQHandler;       // TIM1 Trigger and Commutation interrupt 
    void* pfnTIM1_CC_IRQHandler;            // TIM1 Capture Compare         
    void* pfnTIM2_IRQHandler;               // TIM2                         
    void* pfnTIM3_IRQHandler;               // TIM3                         
    void* pfnTIM4_IRQHandler;               // TIM4                         
    void* pfnI2C1_EV_IRQHandler;            // I2C1 Event                   
    void* pfnI2C1_ER_IRQHandler;            // I2C1 Error                   
    void* pfnI2C2_EV_IRQHandler;            // I2C2 Event                   
    void* pfnI2C2_ER_IRQHandler;            // I2C2 Error                   
    void* pfnSPI1_IRQHandler;               // SPI1                         
    void* pfnSPI2_IRQHandler;               // SPI2                         
    void* pfnUSART1_IRQHandler;             // USART1                       
    void* pfnUSART2_IRQHandler;             // USART2                       
    void* pfnUSART3_IRQHandler;             // USART3                       
    void* pfnEXTI15_10_IRQHandler;          // External Line[15:10]s        
    void* pfnRTC_Alarm_IRQHandler;          // RTC Alarm (A and B) through EXTI Line 
    void* pvReservedC10;                    // Reserved                     
    void* pfnTIM8_BRK_TIM12_IRQHandler;     // TIM8 Break and TIM12         
    void* pfnTIM8_UP_TIM13_IRQHandler;      // TIM8 Update and TIM13        
    void* pfnTIM8_TRG_COM_TIM14_IRQHandler; // TIM8 Trigger and Commutation and TIM14 
    void* pfnTIM8_CC_IRQHandler;            // TIM8 Capture Compare         
    void* pfnDMA1_Stream7_IRQHandler;       // DMA1 Stream7                 
    void* pfnFMC_IRQHandler;                // FMC                          
    void* pfnSDMMC1_IRQHandler;             // SDMMC1                       
    void* pfnTIM5_IRQHandler;               // TIM5                         
    void* pfnSPI3_IRQHandler;               // SPI3                         
    void* pfnUART4_IRQHandler;              // UART4                        
    void* pfnUART5_IRQHandler;              // UART5                        
    void* pfnTIM6_DAC_IRQHandler;           // TIM6 and DAC1&2 underrun errors 
    void* pfnTIM7_IRQHandler;               // TIM7                         
    void* pfnDMA2_Stream0_IRQHandler;       // DMA2 Stream 0                
    void* pfnDMA2_Stream1_IRQHandler;       // DMA2 Stream 1                
    void* pfnDMA2_Stream2_IRQHandler;       // DMA2 Stream 2                
    void* pfnDMA2_Stream3_IRQHandler;       // DMA2 Stream 3                
    void* pfnDMA2_Stream4_IRQHandler;       // DMA2 Stream 4                
    void* pfnETH_IRQHandler;                // Ethernet                     
    void* pfnETH_WKUP_IRQHandler;           // Ethernet Wakeup through EXTI line 
    void* pfnFDCAN_CAL_IRQHandler;          // FDCAN calibration unit interrupt
    void* pvReservedC11;                    // Reserved                     
    void* pvReservedC12;                    // Reserved                     
    void* pvReservedC13;                    // Reserved                     
    void* pvReservedC14;                    // Reserved                     
    void* pfnDMA2_Stream5_IRQHandler;       // DMA2 Stream 5                
    void* pfnDMA2_Stream6_IRQHandler;       // DMA2 Stream 6                
    void* pfnDMA2_Stream7_IRQHandler;       // DMA2 Stream 7                
    void* pfnUSART6_IRQHandler;             // USART6                       
    void* pfnI2C3_EV_IRQHandler;            // I2C3 event                   
    void* pfnI2C3_ER_IRQHandler;            // I2C3 error                   
    void* pfnOTG_HS_EP1_OUT_IRQHandler;     // USB OTG HS End Point 1 Out   
    void* pfnOTG_HS_EP1_IN_IRQHandler;      // USB OTG HS End Point 1 In    
    void* pfnOTG_HS_WKUP_IRQHandler;        // USB OTG HS Wakeup through EXTI 
    void* pfnOTG_HS_IRQHandler;             // USB OTG HS                   
    void* pfnDCMI_IRQHandler;               // DCMI                         
    void* pvReservedC15;                    // Reserved                     
    void* pfnRNG_IRQHandler;                // Rng                          
    void* pfnFPU_IRQHandler;                // FPU                          
    void* pfnUART7_IRQHandler;              // UART7                        
    void* pfnUART8_IRQHandler;              // UART8                        
    void* pfnSPI4_IRQHandler;               // SPI4                         
    void* pfnSPI5_IRQHandler;               // SPI5                         
    void* pfnSPI6_IRQHandler;               // SPI6                         
    void* pfnSAI1_IRQHandler;               // SAI1                         
    void* pfnLTDC_IRQHandler;               // LTDC                         
    void* pfnLTDC_ER_IRQHandler;            // LTDC error                   
    void* pfnDMA2D_IRQHandler;              // DMA2D                        
    void* pfnSAI2_IRQHandler;               // SAI2                         
    void* pfnQUADSPI_IRQHandler;            // QUADSPI                      
    void* pfnLPTIM1_IRQHandler;             // LPTIM1                       
    void* pfnCEC_IRQHandler;                // HDMI_CEC                     
    void* pfnI2C4_EV_IRQHandler;            // I2C4 Event                   
    void* pfnI2C4_ER_IRQHandler;            // I2C4 Error                   
    void* pfnSPDIF_RX_IRQHandler;           // SPDIF_RX                     
    void* pfnOTG_FS_EP1_OUT_IRQHandler;     // USB OTG FS End Point 1 Out   
    void* pfnOTG_FS_EP1_IN_IRQHandler;      // USB OTG FS End Point 1 In    
    void* pfnOTG_FS_WKUP_IRQHandler;        // USB OTG FS Wakeup through EXTI 
    void* pfnOTG_FS_IRQHandler;             // USB OTG FS                   
    void* pfnDMAMUX1_OVR_IRQHandler;        // DMAMUX1 Overrun interrupt    
    void* pfnHRTIM1_Master_IRQHandler;      // HRTIM Master Timer global Interrupt 
    void* pfnHRTIM1_TIMA_IRQHandler;        // HRTIM Timer A global Interrupt 
    void* pfnHRTIM1_TIMB_IRQHandler;        // HRTIM Timer B global Interrupt 
    void* pfnHRTIM1_TIMC_IRQHandler;        // HRTIM Timer C global Interrupt 
    void* pfnHRTIM1_TIMD_IRQHandler;        // HRTIM Timer D global Interrupt 
    void* pfnHRTIM1_TIME_IRQHandler;        // HRTIM Timer E global Interrupt 
    void* pfnHRTIM1_FLT_IRQHandler;         // HRTIM Fault global Interrupt   
    void* pfnDFSDM1_FLT0_IRQHandler;        // DFSDM Filter0 Interrupt        
    void* pfnDFSDM1_FLT1_IRQHandler;        // DFSDM Filter1 Interrupt        
    void* pfnDFSDM1_FLT2_IRQHandler;        // DFSDM Filter2 Interrupt        
    void* pfnDFSDM1_FLT3_IRQHandler;        // DFSDM Filter3 Interrupt        
    void* pfnSAI3_IRQHandler;               // SAI3 global Interrupt          
    void* pfnSWPMI1_IRQHandler;             // Serial Wire Interface 1 global interrupt 
    void* pfnTIM15_IRQHandler;              // TIM15 global Interrupt      
    void* pfnTIM16_IRQHandler;              // TIM16 global Interrupt      
    void* pfnTIM17_IRQHandler;              // TIM17 global Interrupt      
    void* pfnMDIOS_WKUP_IRQHandler;         // MDIOS Wakeup  Interrupt     
    void* pfnMDIOS_IRQHandler;              // MDIOS global Interrupt      
    void* pfnJPEG_IRQHandler;               // JPEG global Interrupt       
    void* pfnMDMA_IRQHandler;               // MDMA global Interrupt       
    void* pvReservedC16;                    // Reserved                    
    void* pfnSDMMC2_IRQHandler;             // SDMMC2 global Interrupt     
    void* pfnHSEM1_IRQHandler;              // HSEM1 global Interrupt      
    void* pvReservedC17;                    // Reserved                    
    void* pfnADC3_IRQHandler;               // ADC3 global Interrupt       
    void* pfnDMAMUX2_OVR_IRQHandler;        // DMAMUX Overrun interrupt    
    void* pfnBDMA_Channel0_IRQHandler;      // BDMA Channel 0 global Interrupt 
    void* pfnBDMA_Channel1_IRQHandler;      // BDMA Channel 1 global Interrupt 
    void* pfnBDMA_Channel2_IRQHandler;      // BDMA Channel 2 global Interrupt 
    void* pfnBDMA_Channel3_IRQHandler;      // BDMA Channel 3 global Interrupt 
    void* pfnBDMA_Channel4_IRQHandler;      // BDMA Channel 4 global Interrupt 
    void* pfnBDMA_Channel5_IRQHandler;      // BDMA Channel 5 global Interrupt 
    void* pfnBDMA_Channel6_IRQHandler;      // BDMA Channel 6 global Interrupt 
    void* pfnBDMA_Channel7_IRQHandler;      // BDMA Channel 7 global Interrupt 
    void* pfnCOMP1_IRQHandler;              // COMP1 global Interrupt     
    void* pfnLPTIM2_IRQHandler;             // LP TIM2 global interrupt   
    void* pfnLPTIM3_IRQHandler;             // LP TIM3 global interrupt   
    void* pfnLPTIM4_IRQHandler;             // LP TIM4 global interrupt   
    void* pfnLPTIM5_IRQHandler;             // LP TIM5 global interrupt   
    void* pfnLPUART1_IRQHandler;            // LP UART1 interrupt         
    void* pvReservedC18;                    // Reserved                   
    void* pfnCRS_IRQHandler;                // Clock Recovery Global Interrupt 
    void* pfnECC_IRQHandler;                // ECC diagnostic Global Interrupt 
    void* pfnSAI4_IRQHandler;               // SAI4 global interrupt      
    void* pvReservedC19;                    // Reserved                   
    void* pvReservedC20;                    // Reserved                   
    void* pfnWAKEUP_PIN_IRQHandler;         // Interrupt for all 6 wake-up pins 
} DeviceVectors;

// Exception Table
__attribute__((section(".vectors"), used))
const DeviceVectors exception_table = {

    .pvStack = (void*) (&_estack),

    .pfnReset_Handler              = (void*) Reset_Handler,
    .pfnNonMaskableInt_Handler     = (void*) NonMaskableInt_Handler,
    .pfnHardFault_Handler          = (void*) HardFault_Handler,
    .pfnMemoryManagement_Handler   = (void*) MemoryManagement_Handler,
    .pfnBusFault_Handler           = (void*) BusFault_Handler,
    .pfnUsageFault_Handler         = (void*) UsageFault_Handler,
    .pvReservedC9                  = nullptr,                       // Reserved
    .pvReservedC8                  = nullptr,                       // Reserved
    .pvReservedC7                  = nullptr,                       // Reserved
    .pvReservedC6                  = nullptr,                       // Reserved
    .pfnSVCall_Handler             = (void*) SVCall_Handler,
    .pfnDebugMonitor_Handler       = (void*) DebugMonitor_Handler,
    .pvReservedC3                  = (void*) (0UL),                 // Reserved
    .pfnPendSV_Handler             = (void*) PendSV_Handler,
    .pfnSysTick_Handler            = (void*) SysTick_Handler,

    // Configurable interrupts.
    .pfnWWDG_IRQHandler               = (void*) KernelHandleIRQ,    // Window WatchDog              
    .pfnPVD_AVD_IRQHandler            = (void*) KernelHandleIRQ,    // PVD/AVD through EXTI Line detection 
    .pfnTAMP_STAMP_IRQHandler         = (void*) KernelHandleIRQ,    // Tamper and TimeStamps through the EXTI line 
    .pfnRTC_WKUP_IRQHandler           = (void*) KernelHandleIRQ,    // RTC Wakeup through the EXTI line 
    .pfnFLASH_IRQHandler              = (void*) KernelHandleIRQ,    // FLASH                        
    .pfnRCC_IRQHandler                = (void*) KernelHandleIRQ,    // RCC                          
    .pfnEXTI0_IRQHandler              = (void*) KernelHandleIRQ,    // EXTI Line0                   
    .pfnEXTI1_IRQHandler              = (void*) KernelHandleIRQ,    // EXTI Line1                   
    .pfnEXTI2_IRQHandler              = (void*) KernelHandleIRQ,    // EXTI Line2                   
    .pfnEXTI3_IRQHandler              = (void*) KernelHandleIRQ,    // EXTI Line3                   
    .pfnEXTI4_IRQHandler              = (void*) KernelHandleIRQ,    // EXTI Line4                   
    .pfnDMA1_Stream0_IRQHandler       = (void*) KernelHandleIRQ,    // DMA1 Stream 0                
    .pfnDMA1_Stream1_IRQHandler       = (void*) KernelHandleIRQ,    // DMA1 Stream 1                
    .pfnDMA1_Stream2_IRQHandler       = (void*) KernelHandleIRQ,    // DMA1 Stream 2                
    .pfnDMA1_Stream3_IRQHandler       = (void*) KernelHandleIRQ,    // DMA1 Stream 3                
    .pfnDMA1_Stream4_IRQHandler       = (void*) KernelHandleIRQ,    // DMA1 Stream 4                
    .pfnDMA1_Stream5_IRQHandler       = (void*) KernelHandleIRQ,    // DMA1 Stream 5                
    .pfnDMA1_Stream6_IRQHandler       = (void*) KernelHandleIRQ,    // DMA1 Stream 6                
    .pfnADC_IRQHandler                = (void*) KernelHandleIRQ,    // ADC1, ADC2 and ADC3s         
    .pfnFDCAN1_IT0_IRQHandler         = (void*) KernelHandleIRQ,    // FDCAN1 interrupt line 0      
    .pfnFDCAN2_IT0_IRQHandler         = (void*) KernelHandleIRQ,    // FDCAN2 interrupt line 0      
    .pfnFDCAN1_IT1_IRQHandler         = (void*) KernelHandleIRQ,    // FDCAN1 interrupt line 1      
    .pfnFDCAN2_IT1_IRQHandler         = (void*) KernelHandleIRQ,    // FDCAN2 interrupt line 1      
    .pfnEXTI9_5_IRQHandler            = (void*) KernelHandleIRQ,    // External Line[9:5]s          
    .pfnTIM1_BRK_IRQHandler           = (void*) KernelHandleIRQ,    // TIM1 Break interrupt         
    .pfnTIM1_UP_IRQHandler            = (void*) KernelHandleIRQ,    // TIM1 Update interrupt        
    .pfnTIM1_TRG_COM_IRQHandler       = (void*) KernelHandleIRQ,    // TIM1 Trigger and Commutation interrupt 
    .pfnTIM1_CC_IRQHandler            = (void*) KernelHandleIRQ,    // TIM1 Capture Compare         
    .pfnTIM2_IRQHandler               = (void*) KernelHandleIRQ,    // TIM2                         
    .pfnTIM3_IRQHandler               = (void*) KernelHandleIRQ,    // TIM3                         
    .pfnTIM4_IRQHandler               = (void*) KernelHandleIRQ,    // TIM4                         
    .pfnI2C1_EV_IRQHandler            = (void*) KernelHandleIRQ,    // I2C1 Event                   
    .pfnI2C1_ER_IRQHandler            = (void*) KernelHandleIRQ,    // I2C1 Error                   
    .pfnI2C2_EV_IRQHandler            = (void*) KernelHandleIRQ,    // I2C2 Event                   
    .pfnI2C2_ER_IRQHandler            = (void*) KernelHandleIRQ,    // I2C2 Error                   
    .pfnSPI1_IRQHandler               = (void*) KernelHandleIRQ,    // SPI1                         
    .pfnSPI2_IRQHandler               = (void*) KernelHandleIRQ,    // SPI2                         
    .pfnUSART1_IRQHandler             = (void*) KernelHandleIRQ,    // USART1                       
    .pfnUSART2_IRQHandler             = (void*) KernelHandleIRQ,    // USART2                       
    .pfnUSART3_IRQHandler             = (void*) KernelHandleIRQ,    // USART3                       
    .pfnEXTI15_10_IRQHandler          = (void*) KernelHandleIRQ,    // External Line[15:10]s        
    .pfnRTC_Alarm_IRQHandler          = (void*) KernelHandleIRQ,    // RTC Alarm (A and B) through EXTI Line 
    .pvReservedC10                    = (void*) (0UL),              // Reserved                     
    .pfnTIM8_BRK_TIM12_IRQHandler     = (void*) KernelHandleIRQ,    // TIM8 Break and TIM12         
    .pfnTIM8_UP_TIM13_IRQHandler      = (void*) KernelHandleIRQ,    // TIM8 Update and TIM13        
    .pfnTIM8_TRG_COM_TIM14_IRQHandler = (void*) KernelHandleIRQ,    // TIM8 Trigger and Commutation and TIM14 
    .pfnTIM8_CC_IRQHandler            = (void*) KernelHandleIRQ,    // TIM8 Capture Compare         
    .pfnDMA1_Stream7_IRQHandler       = (void*) KernelHandleIRQ,    // DMA1 Stream7                 
    .pfnFMC_IRQHandler                = (void*) KernelHandleIRQ,    // FMC                          
    .pfnSDMMC1_IRQHandler             = (void*) KernelHandleIRQ,    // SDMMC1                       
    .pfnTIM5_IRQHandler               = (void*) KernelHandleIRQ,    // TIM5                         
    .pfnSPI3_IRQHandler               = (void*) KernelHandleIRQ,    // SPI3                         
    .pfnUART4_IRQHandler              = (void*) KernelHandleIRQ,    // UART4                        
    .pfnUART5_IRQHandler              = (void*) KernelHandleIRQ,    // UART5                        
    .pfnTIM6_DAC_IRQHandler           = (void*) KernelHandleIRQ,    // TIM6 and DAC1&2 underrun errors 
    .pfnTIM7_IRQHandler               = (void*) KernelHandleIRQ,    // TIM7                         
    .pfnDMA2_Stream0_IRQHandler       = (void*) KernelHandleIRQ,    // DMA2 Stream 0                
    .pfnDMA2_Stream1_IRQHandler       = (void*) KernelHandleIRQ,    // DMA2 Stream 1                
    .pfnDMA2_Stream2_IRQHandler       = (void*) KernelHandleIRQ,    // DMA2 Stream 2                
    .pfnDMA2_Stream3_IRQHandler       = (void*) KernelHandleIRQ,    // DMA2 Stream 3                
    .pfnDMA2_Stream4_IRQHandler       = (void*) KernelHandleIRQ,    // DMA2 Stream 4                
    .pfnETH_IRQHandler                = (void*) KernelHandleIRQ,    // Ethernet                     
    .pfnETH_WKUP_IRQHandler           = (void*) KernelHandleIRQ,    // Ethernet Wakeup through EXTI line 
    .pfnFDCAN_CAL_IRQHandler          = (void*) KernelHandleIRQ,    // FDCAN calibration unit interrupt
    .pvReservedC11                    = (void*) (0UL),              // Reserved                     
    .pvReservedC12                    = (void*) (0UL),              // Reserved                     
    .pvReservedC13                    = (void*) (0UL),              // Reserved                     
    .pvReservedC14                    = (void*) (0UL),              // Reserved                     
    .pfnDMA2_Stream5_IRQHandler       = (void*) KernelHandleIRQ,    // DMA2 Stream 5                
    .pfnDMA2_Stream6_IRQHandler       = (void*) KernelHandleIRQ,    // DMA2 Stream 6                
    .pfnDMA2_Stream7_IRQHandler       = (void*) KernelHandleIRQ,    // DMA2 Stream 7                
    .pfnUSART6_IRQHandler             = (void*) KernelHandleIRQ,    // USART6                       
    .pfnI2C3_EV_IRQHandler            = (void*) KernelHandleIRQ,    // I2C3 event                   
    .pfnI2C3_ER_IRQHandler            = (void*) KernelHandleIRQ,    // I2C3 error                   
    .pfnOTG_HS_EP1_OUT_IRQHandler     = (void*) KernelHandleIRQ,    // USB OTG HS End Point 1 Out   
    .pfnOTG_HS_EP1_IN_IRQHandler      = (void*) KernelHandleIRQ,    // USB OTG HS End Point 1 In    
    .pfnOTG_HS_WKUP_IRQHandler        = (void*) KernelHandleIRQ,    // USB OTG HS Wakeup through EXTI 
    .pfnOTG_HS_IRQHandler             = (void*) KernelHandleIRQ,    // USB OTG HS                   
    .pfnDCMI_IRQHandler               = (void*) KernelHandleIRQ,    // DCMI                         
    .pvReservedC15                    = (void*) (0UL),              // Reserved                     
    .pfnRNG_IRQHandler                = (void*) KernelHandleIRQ,    // Rng                          
    .pfnFPU_IRQHandler                = (void*) KernelHandleIRQ,    // FPU                          
    .pfnUART7_IRQHandler              = (void*) KernelHandleIRQ,    // UART7                        
    .pfnUART8_IRQHandler              = (void*) KernelHandleIRQ,    // UART8                        
    .pfnSPI4_IRQHandler               = (void*) KernelHandleIRQ,    // SPI4                         
    .pfnSPI5_IRQHandler               = (void*) KernelHandleIRQ,    // SPI5                         
    .pfnSPI6_IRQHandler               = (void*) KernelHandleIRQ,    // SPI6                         
    .pfnSAI1_IRQHandler               = (void*) KernelHandleIRQ,    // SAI1                         
    .pfnLTDC_IRQHandler               = (void*) KernelHandleIRQ,    // LTDC                         
    .pfnLTDC_ER_IRQHandler            = (void*) KernelHandleIRQ,    // LTDC error                   
    .pfnDMA2D_IRQHandler              = (void*) KernelHandleIRQ,    // DMA2D                        
    .pfnSAI2_IRQHandler               = (void*) KernelHandleIRQ,    // SAI2                         
    .pfnQUADSPI_IRQHandler            = (void*) KernelHandleIRQ,    // QUADSPI                      
    .pfnLPTIM1_IRQHandler             = (void*) KernelHandleIRQ,    // LPTIM1                       
    .pfnCEC_IRQHandler                = (void*) KernelHandleIRQ,    // HDMI_CEC                     
    .pfnI2C4_EV_IRQHandler            = (void*) KernelHandleIRQ,    // I2C4 Event                   
    .pfnI2C4_ER_IRQHandler            = (void*) KernelHandleIRQ,    // I2C4 Error                   
    .pfnSPDIF_RX_IRQHandler           = (void*) KernelHandleIRQ,    // SPDIF_RX                     
    .pfnOTG_FS_EP1_OUT_IRQHandler     = (void*) KernelHandleIRQ,    // USB OTG FS End Point 1 Out   
    .pfnOTG_FS_EP1_IN_IRQHandler      = (void*) KernelHandleIRQ,    // USB OTG FS End Point 1 In    
    .pfnOTG_FS_WKUP_IRQHandler        = (void*) KernelHandleIRQ,    // USB OTG FS Wakeup through EXTI 
    .pfnOTG_FS_IRQHandler             = (void*) KernelHandleIRQ,    // USB OTG FS                   
    .pfnDMAMUX1_OVR_IRQHandler        = (void*) KernelHandleIRQ,    // DMAMUX1 Overrun interrupt    
    .pfnHRTIM1_Master_IRQHandler      = (void*) KernelHandleIRQ,    // HRTIM Master Timer global Interrupt 
    .pfnHRTIM1_TIMA_IRQHandler        = (void*) KernelHandleIRQ,    // HRTIM Timer A global Interrupt 
    .pfnHRTIM1_TIMB_IRQHandler        = (void*) KernelHandleIRQ,    // HRTIM Timer B global Interrupt 
    .pfnHRTIM1_TIMC_IRQHandler        = (void*) KernelHandleIRQ,    // HRTIM Timer C global Interrupt 
    .pfnHRTIM1_TIMD_IRQHandler        = (void*) KernelHandleIRQ,    // HRTIM Timer D global Interrupt 
    .pfnHRTIM1_TIME_IRQHandler        = (void*) KernelHandleIRQ,    // HRTIM Timer E global Interrupt 
    .pfnHRTIM1_FLT_IRQHandler         = (void*) KernelHandleIRQ,    // HRTIM Fault global Interrupt   
    .pfnDFSDM1_FLT0_IRQHandler        = (void*) KernelHandleIRQ,    // DFSDM Filter0 Interrupt        
    .pfnDFSDM1_FLT1_IRQHandler        = (void*) KernelHandleIRQ,    // DFSDM Filter1 Interrupt        
    .pfnDFSDM1_FLT2_IRQHandler        = (void*) KernelHandleIRQ,    // DFSDM Filter2 Interrupt        
    .pfnDFSDM1_FLT3_IRQHandler        = (void*) KernelHandleIRQ,    // DFSDM Filter3 Interrupt        
    .pfnSAI3_IRQHandler               = (void*) KernelHandleIRQ,    // SAI3 global Interrupt          
    .pfnSWPMI1_IRQHandler             = (void*) KernelHandleIRQ,    // Serial Wire Interface 1 global interrupt 
    .pfnTIM15_IRQHandler              = (void*) KernelHandleIRQ,    // TIM15 global Interrupt      
    .pfnTIM16_IRQHandler              = (void*) KernelHandleIRQ,    // TIM16 global Interrupt      
    .pfnTIM17_IRQHandler              = (void*) KernelHandleIRQ,    // TIM17 global Interrupt      
    .pfnMDIOS_WKUP_IRQHandler         = (void*) KernelHandleIRQ,    // MDIOS Wakeup  Interrupt     
    .pfnMDIOS_IRQHandler              = (void*) KernelHandleIRQ,    // MDIOS global Interrupt      
    .pfnJPEG_IRQHandler               = (void*) KernelHandleIRQ,    // JPEG global Interrupt       
    .pfnMDMA_IRQHandler               = (void*) KernelHandleIRQ,    // MDMA global Interrupt       
    .pvReservedC16                    = (void*) (0UL),              // Reserved                    
    .pfnSDMMC2_IRQHandler             = (void*) KernelHandleIRQ,    // SDMMC2 global Interrupt     
    .pfnHSEM1_IRQHandler              = (void*) KernelHandleIRQ,    // HSEM1 global Interrupt      
    .pvReservedC17                    = (void*) (0UL),              // Reserved                    
    .pfnADC3_IRQHandler               = (void*) KernelHandleIRQ,    // ADC3 global Interrupt       
    .pfnDMAMUX2_OVR_IRQHandler        = (void*) KernelHandleIRQ,    // DMAMUX Overrun interrupt    
    .pfnBDMA_Channel0_IRQHandler      = (void*) KernelHandleIRQ,    // BDMA Channel 0 global Interrupt 
    .pfnBDMA_Channel1_IRQHandler      = (void*) KernelHandleIRQ,    // BDMA Channel 1 global Interrupt 
    .pfnBDMA_Channel2_IRQHandler      = (void*) KernelHandleIRQ,    // BDMA Channel 2 global Interrupt 
    .pfnBDMA_Channel3_IRQHandler      = (void*) KernelHandleIRQ,    // BDMA Channel 3 global Interrupt 
    .pfnBDMA_Channel4_IRQHandler      = (void*) KernelHandleIRQ,    // BDMA Channel 4 global Interrupt 
    .pfnBDMA_Channel5_IRQHandler      = (void*) KernelHandleIRQ,    // BDMA Channel 5 global Interrupt 
    .pfnBDMA_Channel6_IRQHandler      = (void*) KernelHandleIRQ,    // BDMA Channel 6 global Interrupt 
    .pfnBDMA_Channel7_IRQHandler      = (void*) KernelHandleIRQ,    // BDMA Channel 7 global Interrupt 
    .pfnCOMP1_IRQHandler              = (void*) KernelHandleIRQ,    // COMP1 global Interrupt     
    .pfnLPTIM2_IRQHandler             = (void*) KernelHandleIRQ,    // LP TIM2 global interrupt   
    .pfnLPTIM3_IRQHandler             = (void*) KernelHandleIRQ,    // LP TIM3 global interrupt   
    .pfnLPTIM4_IRQHandler             = (void*) KernelHandleIRQ,    // LP TIM4 global interrupt   
    .pfnLPTIM5_IRQHandler             = (void*) KernelHandleIRQ,    // LP TIM5 global interrupt   
    .pfnLPUART1_IRQHandler            = (void*) KernelHandleIRQ,    // LP UART1 interrupt         
    .pvReservedC18                    = (void*) (0UL),              // Reserved                   
    .pfnCRS_IRQHandler                = (void*) KernelHandleIRQ,    // Clock Recovery Global Interrupt 
    .pfnECC_IRQHandler                = (void*) KernelHandleIRQ,    // ECC diagnostic Global Interrupt 
    .pfnSAI4_IRQHandler               = (void*) KernelHandleIRQ,    // SAI4 global interrupt      
    .pvReservedC19                    = (void*) (0UL),              // Reserved                   
    .pvReservedC20                    = (void*) (0UL),              // Reserved                   
    .pfnWAKEUP_PIN_IRQHandler         = (void*) KernelHandleIRQ,    // Interrupt for all 6 wake-up pins 
};

} // extern "C"
