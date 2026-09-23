# Profiler sampling

Enable sampling with `PADOS_MODULE_GPROF_SAMPLING`. The following CMake cache
settings select the sampling interrupt:

| Setting | PadOS default | Meaning |
| --- | --- | --- |
| `PADOS_GPROF_SAMPLING_TIMER` | `SysTick` | `SysTick`, `TIM1` through `TIM8`, or `TIM12` through `TIM17`. Hardware timers require STM32H7. |
| `PADOS_GPROF_SAMPLE_RATE_HZ` | `500` | Requested sampling frequency. SysTick sampling is limited to the system tick rate. |
| `PADOS_GPROF_IRQ_PRIORITY` | `1` | Hardware timer NVIC priority, from 0 (highest) to 15 (lowest) on STM32H7. Ignored for SysTick. |

The cache settings can be overridden by a board configuration or CMake preset.
Select an unused hardware timer for the target system, or use `SysTick` when no
spare timer is available.

A hardware source reserves the complete timer and its update IRQ for the profiler,
even while capture is stopped. Other drivers cannot register a handler on that IRQ.
For shared IRQs, every source on the vector must be reserved:
TIM6 shares with the DAC, TIM8 update shares with TIM13, TIM12 shares with TIM8
break, and TIM14 shares with TIM8 trigger/commutation. The board configuration must
also ensure that no other driver uses the selected timer's channels.

The timer runs only during capture and pauses when the debugger halts the CPU.
Its interrupt samples the stacked program counter from either thread or handler
mode. It can preempt lower-priority interrupts, subject to the processor's active
interrupt masks. It cannot sample an interrupt at the same or a higher priority.

Use a frequency such as 997 Hz to move the hardware samples through the 1 kHz
SysTick cycle. A different fixed frequency reduces SysTick phase locking; it does
not eliminate every possible alias with periodic work. SysTick sampling retains
its phase accumulator and remains tied to system ticks.

Timer divisors are rounded to a representable period. Profile status and gmon
histogram headers report the resulting frequency rounded to integer hertz, as
required by the gmon format.
