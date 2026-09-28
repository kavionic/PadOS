# PadOS thread debugger interface

The C-linkage symbol _kernel_debugger_info is a read-only descriptor for debuggers that
inspect halted targets. Include/Kernel/KDebugger.h is the authoritative definition
of its versioned layout. Version 1 supports the STM32H7 Cortex-M context format.
Other builds advertise an unsupported architecture and must be rejected.

All integers and pointers use target byte order. Version 1 has 32-bit pointers,
a 116-byte descriptor, and 16-bit register offsets. A reader must check the
version, descriptor size, pointer size, and architecture before reading threads.
An incompatible change requires a new interface version.

## Enumerating threads

Threads points to the PIntrusiveList used for the debugger registry.
ThreadListFirstOffset locates its first-node pointer. Subtract ThreadNodeOffset
from each non-null node pointer to obtain its KThreadCB address, then read the
next-node pointer at ThreadNextOffset from that thread. Traverse until null,
using an independent limit to stop corrupted or cyclic lists. The list's count
is not part of the debugger interface. All offsets are derived from the actual
intrusive-list and thread types.

List mutation happens only during context initialization and destruction, under
the scheduler lock. Forward links publish initialized nodes and bypass removed
nodes before their links are cleared or their storage is released. Compiler
barriers preserve that ordering for inspection of the halted processor. Readers
follow forward links only, so they do not depend on updates to the backward
links or count having finished. There is no additional work on an ordinary
context switch.

The list contains threads with initialized contexts, including unreaped zombies.
Use the exported state table to interpret the actual state values. The name,
state, priority level, saved stack pointer, and syscall return address are read
from the thread itself using the descriptor's offsets. Names are UTF-8, bounded
by ThreadNameSize. ThreadStateSize gives the width of the state field, which
may be smaller than 32 bits. Add PriorityMinimum to the stored priority level.

CurrentThread points to the scheduler's current-thread pointer. Its live
registers come from the processor, not its old saved context. Before scheduler
startup, the list can be empty or omit the bootstrap idle thread; a debugger
must retain ordinary single-context debugging during that period.

The idle thread's PadOS ID is zero. Since GDB reserves thread ID zero, OpenOCD
maps idle to protocol ID 0x7fffffff and maps that ID back to zero when locating
its control block. All other thread IDs are passed through unchanged. The PID
allocator reserves std::numeric_limits<pid_t>::max() for this purpose, allocating
only through std::numeric_limits<pid_t>::max() - 1 (0x7ffffffe on this target).
Before bootstrap idle is registered, the same protocol ID represents the
processor's live execution. GDB's own thread numbers remain independent of
PadOS IDs.

## Reading saved registers

Clear privilege bits using StackPointerMask before reading the saved context.
Read EXC_RETURN at ExceptionReturnOffset: bit 4 selects the basic frame when
set and the extended floating-point frame when clear. The descriptor provides
the frame sizes and core-register offset tables derived from KStackFrames.h.

The tables describe R0-R12, SP, LR, PC, and xPSR. The SP entry is a sentinel:
recover SP by adding the chosen frame size and, when saved xPSR bit 9 is set,
four bytes of alignment padding. The extended table accounts for both the
software-saved and hardware-saved floating-point areas.

FloatingPointRegisterOffsets contains FloatingPointRegisterCount (17) offsets
within the extended frame: D0-D15 followed by FPSCR. Each D register consists of
two consecutive 32-bit S registers, with the low word first; FPSCR is one word.
Decode each word in target byte order before assembling a 64-bit D register.
These offsets are derived from the same KStackFrames.h definitions as the core
register tables. A basic frame has no saved floating-point context.

The scheduler's VSTM instruction saves S16-S31 and completes lazy preservation
of S0-S15 and FPSCR before publishing the suspended thread's stack pointer.
OpenOCD reads D0-D15 and FPSCR through individual register requests; GDB derives
S0-S31 from the D registers. Unsaved registers must not be read from the current
thread's processor state when a different thread is selected.

ThreadSyscallReturnOffset identifies the saved userspace return address needed
by the PadOS syscall unwinder. A GDB unwinder must use the selected thread's
control block instead of assuming it is always the scheduler's current thread.

## Retention and cost

The descriptor has static initialization. Its translation unit belongs to
PadOS_Kernel_Unconditional, whose interface link options make _kernel_debugger_info
a linker root. This preserves it during archive extraction and section garbage
collection. The used attribute also prevents compiler elimination. No firmware
call into the debugger is needed. The registry adds three pointers per thread and a 12-byte list;
metadata, register offsets, and state names are read-only.
