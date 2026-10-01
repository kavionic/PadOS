# PadOS

PadOS is an embedded operating system written primarily in C++23 for ARM microcontrollers. It combines a preemptive multitasking kernel with a graphical application framework, filesystems, USB connectivity, and hardware drivers.

PadOS supports devices that combine hardware control with interactive graphical interfaces. Applications can use C++ classes for threads, messaging, storage, and GUI development, alongside POSIX-style system APIs. Including POSIX threads.

The current build configuration targets STM32H7 Cortex-M7 microcontrollers.

[API documentation](https://kavionic.github.io/PadOS/classes.html)

## Features

### Kernel and multitasking

- **Preemptive, priority-based scheduling** with 32 priority levels and round-robin scheduling between runnable threads of equal priority.
- **Thread management** with configurable priorities and stacks, joinable and detached threads, thread-local storage, and execution-time accounting.
- **Synchronization primitives** including mutexes, shared/exclusive locking, semaphores, and condition variables, with timed waits.
- **Object wait groups** for waiting on multiple kernel objects and file descriptors without application-level polling.
- **Message ports** for communication between threads and services.
- **Monotonic and real-time clocks**, timed sleeps, and event timers.
- **Interrupt handling** with immediate and deferred callbacks.
- **Optional user-mode execution** with system-call dispatch into the kernel.

### Processes and application execution

PadOS provides process and terminal facilities suited to an embedded environment:

- Process creation through `posix_spawn()`.
- Parent/child relationships, exit status, and process waiting.
- Process groups, sessions, and terminal job control.
- POSIX signal handling and thread cancellation.
- Pipes and pseudo-terminals (PTY's).
- Per-process file descriptor management.

Applications are registered and linked into the firmware. The virtual BinFS filesystem exposes these built-in applications as executable entries that can be launched by name.

PadOS implements a selection of POSIX interfaces; support should be assessed against the APIs required by each application.

### Graphics and GUI

PadOS includes an application server, window manager, and C++ widget toolkit.

- **Window and view management** with a hierarchical view model, clipping, invalidation, and drawing commands.
- **Layouts defined in C++ or XML**, with horizontal and vertical arrangements, preferred sizes, alignment, spacing, and weighted sizing.
- **Widgets** including buttons, checkboxes, radio buttons, sliders, progress bars, text controls, menus, dropdowns, tabs, scroll views, lists, and grids.
- **Model/view controls** for displaying application data with custom item widgets and selection handling.
- **Dialogs** for messages, errors, and text entry.
- **Touch, mouse, and keyboard input**, including pointer capture, event capture and bubbling, taps, long presses, and mouse-wheel events.
- **On-screen keyboards** with alphanumeric and numeric input modes.
- **Animation helpers** for value interpolation and inertial scrolling.
- **Bitmap and text rendering**, bitmap scaling with bilinear filtering, and PNG image decoding.
- **RA8875 display support**, including hardware-assisted drawing and hardware mouse cursors.

A display-driver interface separates the application server from the display hardware.

### Filesystems and storage

- **Virtual filesystem layer** providing a common interface to mounted filesystems and device nodes.
- **Read/write FAT12, FAT16, and FAT32 support**, including long filenames and conversion between FAT filename encodings and UTF-8.
- File and directory creation, reading, writing, resizing, renaming, and removal.
- Volume mounting, unmounting, synchronization, and filesystem information.
- **Block caching with background writeback** and a transition to read-only operation after device write failures.
- **SD/MMC and USB mass-storage integration**, including partition handling.
- **C++ storage classes** for files, directories, paths, streams, temporary files, and memory-backed files.
- Optional virtual filesystems for built-in applications, pipes, and pseudo-terminals.

### USB

PadOS includes USB host and device stacks with an STM32 hardware backend.

**USB host features:**

- Device enumeration and connection/disconnection handling.
- USB hub support.
- CDC serial communication.
- HID report parsing, keyboards, and mice.
- Mass-storage devices using SCSI commands over Bulk-Only Transport.
- Device and interface nodes for inspection and control.

**USB device features:**

- CDC virtual serial communication.
- A class-driver framework for implementing device functionality.

### Hardware support

The hardware abstraction layer and configurable drivers cover:

| Area | Included support |
| --- | --- |
| General peripherals | GPIO, interrupts, DMA, timers, and ADC |
| Communication | I²C, SPI, USART, and USB |
| Storage and memory | SD/MMC, QSPI flash, and external SDRAM |
| Displays and touch | RA8875, FT5x0x, and GSLx680 |
| Motion control | Multi-motor control and TMC2209 stepper drivers |
| Additional devices | Real-time clock, WS2812B LEDs, TLV493D magnetic sensor, and piezo buzzer |

Driver availability depends on the target hardware and build configuration.

### C++ application framework

- Thread and event-loop classes for asynchronous applications.
- Typed signal/slots type delegates with synchronous or asynchronous delivery (including message-port-based remote signals).
- Reference-counted pointers and weak references implemented using lockless thread-safe updates.
- UTF-8 strings, Unicode conversion, and case-folding utilities.
- XML parsing and object factories.
- Geometry, vector, PID-control, and animation helpers.
- Integration with the PadOS toolchain's C and C++ runtime, including standard-library threading and synchronization facilities.

### Console, diagnostics, and profiling

- **Interactive UNIX-shell like debug console** with line editing, history, command completion, pipelines, and foreground/background job control.
  - The shell can be accessed via a debug UART port or via a multiplexed CDC port using a host-PC SSH server for running multiple shells.
- **File utilities** including `ls`, `cp`, `mv`, `rm`, `mkdir`, `rmdir`, `find`, `grep`, and `less`.
- **System inspection** through commands such as `ps`, `top`, and `ls_usb`.
- **Categorized logging** with configurable severity levels, serial output, and rotating log files.
- **Serial command handlers** for host communication and filesystem operations.
- **Optional GNU gprof profiling** with statistical sampling and instrumented call graphs.
- **Optional GoogleTest suites** covering kernel interfaces, concurrency, process handling, USB behavior, and utility classes.
- **Doxygen integration** for generating API documentation.

## Architecture

PadOS is organized into several cooperating layers:

| Component | Responsibility |
| --- | --- |
| Kernel | Scheduling, synchronization, processes, system calls, interrupts, and device management |
| Storage subsystem | Virtual filesystem, block cache, and storage devices |
| Device filesystem | Dynamic /dev/ filesystem populated by device drivers to expose their interfaces |
| System libraries | C++ interfaces for threading, messaging, storage, and utilities |
| Application server | Display management, rendering, and input routing |
| GUI toolkit | Views, layouts, widgets, dialogs, and application-side event handling |
| Window manager | Window coordination and on-screen keyboard management |
| Applications | Built-in utilities and firmware-specific applications |

## Toolchain, debugging, and profiling

The companion `PadOSToolchain` project provides PadOS ports of GCC, GDB, newlib, and OpenOCD, together with GNU
binutils and gprof. The C/C++ toolchain targets `arm-unknown-pados-eabi` and integrates with PadOS threads,
system calls, and runtime services.

### Compiler and runtime

- **GCC and libstdc++** provide C++23 support and a POSIX thread backend for standard C++ threads, mutexes,
  and condition variables.
- **Native thread-local storage (TLS)** supports C/C++ thread-local variables, with separate kernel and
  user-space TLS blocks for each thread.
- **newlib** supplies the C library and PadOS system-interface headers, with thread-local reentrancy state
  and runtime locks backed by PadOS pthread mutexes.
- **C++ stack cleanup** is supported by PadOS-specific GCC handling of `exit()` and `abort()`, preserving
  cleanup paths for the runtime's forced stack unwinding.

### Thread-aware debugging

The OpenOCD port includes a `PadOS` RTOS backend that reads the kernel's versioned debugger interface. It exposes
thread names, IDs, states, and priorities to GDB and reconstructs saved core and floating-point registers for
suspended threads. This supports thread selection, register inspection, and backtraces across PadOS threads.

The GDB port recognizes PadOS kernel and application ELF images and cooperates with OpenOCD to resolve TLS
variables for the selected thread. Kernel and user-space TLS are distinguished by the variable's image, so
both remain inspectable when a thread is stopped inside a system call.

The supplied [GDB unwinder](https://github.com/kavionic/PadOS/blob/master/Tools/gdb_pados_unwinder.py)
reconstructs the caller across the syscall trampoline,
allowing backtraces to continue from kernel code into the user-space caller. It also terminates backtraces at
the thread entry point. Load it with GDB's `source` command after loading the firmware symbols; use the PadOS
OpenOCD backend (`-rtos PadOS`) and load both kernel and application symbols when debugging a split image.

### Sampling and call-graph profiling

PadOS includes an on-target profiler that exports GNU gprof data for the kernel and application images:

- **Statistical sampling** is enabled with `PADOS_MODULE_GPROF_SAMPLING`. It uses SysTick or a dedicated STM32H7
  hardware timer, with a configurable sample rate. A hardware timer can also sample lower-priority interrupt
  handlers, subject to interrupt masking.
- **Instrumented call graphs** are enabled with `PADOS_MODULE_GPROF_CALL_GRAPH`, which adds GCC's `-pg`
  instrumentation and records caller/callee counts for thread-mode execution. This option requires sampling;
  profiling currently also requires `PADOS_MODULE_USER_SPACE`.
- **Runtime capture control** is available through the debug console's `profile start`, `profile stop`,
  `profile status`, and `profile dump [path-prefix]` commands.
- **Whole-application runs** can be timed and profiled with `time -P <app> [arguments...]` (or `time --profile`).
  This reports elapsed (`real`), user CPU (`user`), and kernel CPU (`sys`) times, starts profiling before launching
  the application, and stops capture when it finishes. Run `profile dump [path-prefix]` afterward to save the data.

Run `profile start`, exercise the workload, then run `profile stop` and `profile dump`. By default, the dump
creates timestamped `-kernel.gmon` and `-application.gmon` files under `/var/profiles`. Copy them to the host
and analyze each file with `arm-unknown-pados-eabi-gprof`, passing the matching kernel or application ELF from
the same build. Sampling provides flat execution-time profiles; instrumented captures also provide call graphs.

See the [profiler configuration notes](Kernel/Profiler/README.md) for timer selection, sampling frequency,
interrupt priority, and hardware reservation requirements.

## Building and integration

PadOS uses CMake with Ninja-based STM32H7 presets and C++23 enabled.

The supplied ARM toolchain configuration expects the PadOS-specific
`arm-unknown-pados-eabi` GCC toolchain, located through `PADOS_TOOLCHAIN_PATH`.
This toolchain supplies PadOS runtime and system-interface headers.

PadOS can be built within a firmware project or installed as CMake libraries.
The integrating firmware supplies board initialization, memory layout, hardware
configuration, and application entry points. External dependencies are maintained
in the companion PadOSExternalLibs project.

CMake options control individual drivers and facilities such as USB host support,
user-space dispatch, POSIX spawning and signals, the debug console, profiling,
and unit tests.

## Platform status

STM32H7 is the target of the supplied main build presets. The repository also
contains some SAME70 platform and peripheral support, but it have not been
maintained in a long time and should be considered broken.

## License

PadOS use the Apache-2.0 license.
