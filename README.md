# PadOS

PadOS is an embedded operating system written primarily in C++23 for ARM microcontrollers. It combines a preemptive multitasking kernel with a graphical application framework, filesystems, USB connectivity, and hardware drivers.

PadOS supports devices that combine hardware control with interactive graphical interfaces. Applications can use C++ classes for threads, messaging, storage, and GUI development, alongside POSIX-style system APIs. Including POSIX threads.

The current build configuration targets STM32H7 Cortex-M7 microcontrollers.

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
