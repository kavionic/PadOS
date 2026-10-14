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

## Local bridge into Visual Studio's GDB

`gdb_pados_bridge.py` runs inside the GDB process that Visual Studio already owns.
It reuses that process's connection, symbols, RTOS threads and Python unwinders.
`pados_gdb_client.py` sends authenticated local requests; it never opens another
GDB connection to OpenOCD. Both files require Python 3.9 or later. The tested
installation uses GDB 18.1 and MinGW Python 3.12.11 on Windows.

### Load, discover and inspect

With the existing session stopped, enter this in Visual Studio's Command Window:

```text
Debug.MIDebugExec source D:/Rainbow3D/Rainbow3DFirmware/PadOS/Tools/gdb_pados_bridge.py
Debug.MIDebugExec pados-bridge status
```

Loading starts a background transport worker. Status reports the endpoint path,
startup errors and lifecycle state; it does not change target execution. The
helper uses the embedded Python installation's `sysconfig` extension directory
and `sys.prefix/bin` DLL directory to correct this GDB distribution's relocated
Python module search paths. It does not alter the system Python installation.

Use the explicit Python 3 executable: bare `python` is Python 2.7 on this host.
From PowerShell:

```powershell
$gdbPython = 'C:\msys64\mingw64\bin\python3.exe'
$gdbClient = 'D:\Rainbow3D\Rainbow3DFirmware\PadOS\Tools\pados_gdb_client.py'
& $gdbPython $gdbClient discover
```

Discovery lists files under `%LOCALAPPDATA%\PadOS\GDBBridge`, with GDB PID, loaded
program, session identity, endpoint path and transport reachability. It does not
print authentication tokens. Choose the entry matching the intended GDB and ELF;
do not hardcode the ephemeral TCP port or assume a PID remains valid after restart.
Set `$gdbEndpoint` to the returned `endpoint` value, then use:

```powershell
& $gdbPython $gdbClient --endpoint $gdbEndpoint request status
& $gdbPython $gdbClient --endpoint $gdbEndpoint request threads
& $gdbPython $gdbClient --endpoint $gdbEndpoint --timeout 15 request stack '{"thread":61,"count":12}'
& $gdbPython $gdbClient --endpoint $gdbEndpoint request evaluate '{"expression":"sizeof(kernel::RA8875GfxDriver)"}'
& $gdbPython $gdbClient --endpoint $gdbEndpoint request locals '{"thread":61,"frame":5}'
& $gdbPython $gdbClient --endpoint $gdbEndpoint request registers '{"thread":61,"names":["pc","sp","lr"]}'
& $gdbPython $gdbClient --endpoint $gdbEndpoint request disassemble '{"thread":61,"count":3}'
```

Thread 61 and frame 5 are examples from the validation session. Enumerate again
for each session. Thread arguments identify GDB global thread numbers, not PadOS
thread IDs. This bridge operates on the selected inferior; it does not implement
multi-inferior switching. Visual Studio can display one thread while GDB's default
selection is another, because MIEngine supplies context with its own requests.
Use explicit `thread`/`frame` arguments for inspections. These temporarily select
the requested context and restore the previous valid GDB thread/frame afterward.

The descriptor directory has a protected Windows DACL granting the current user
and SYSTEM access. Each session also has a random 256-bit authentication token;
the listener binds only to 127.0.0.1. Treat the descriptor as a credential and do
not copy it to the repository or print its token. Codex's restricted Windows
sandbox account cannot read it. Run the client through authorized host execution
as the Visual Studio user, without granting the sandbox account broader access.

### Operations and execution ownership

| Operation | Arguments and behavior |
|---|---|
| `status` | GDB program, selected thread, running state, MI async mode and session identity. |
| `threads` | GDB's thread listing, including target thread IDs and names. |
| `evaluate` | `expression`; returns its value and type. |
| `type` | `expression`; returns `ptype` output. |
| `locals` | Returns arguments and locals for the chosen frame. |
| `registers` | Optional `names` array; defaults to ordinary register listing. |
| `stack` | Optional `count`, default 32, maximum 256. |
| `disassemble` | Optional `address` expression, default `$pc`; optional instruction `count`, default 32, maximum 256. |
| `select` | Explicitly changes GDB's `thread` and/or `frame` selection. |
| `breakpoints` | Lists all breakpoints and identifies bridge-owned numbers. |
| `break` | `location`; optional `temporary`; hardware by default, explicit `hardware:false` for software breakpoints. |
| `watch` | `expression`; `access` is `write` (default), `read` or `access`. GDB/target determine hardware support. |
| `delete`, `enable`, `disable` | `number`; only bridge-owned breakpoints/watchpoints may be modified. |
| `condition` | `number` and optional `expression`; omission clears the condition. |
| `halt` | Requests an interrupt; does not imply the stop has already occurred. |
| `continue`, `step`, `next`, `stepi`, `nexti`, `finish` | Background execution; success means command acceptance, not completion of execution. |
| `shutdown` | Stops transport and cancels queued requests; retains target state and breakpoints. |

Inspection operations accept optional `thread` and `frame`. Except for `status`,
`threads`, `breakpoints`, `halt` and `shutdown`, operations require a stopped
target. The bridge never halts implicitly. Execution commands require a connected
all-stop target; non-stop execution is rejected. Background commands require
target async support. This bridge does not change `mi-async` or `non-stop` settings.
If GDB is busy and cannot service posted events, transport discovery and recorded
request status remain available, but new GDB operations can expire in the queue.

Inspection temporarily disables inferior function calls and memory/register
writes, restoring the original GDB parameters afterward. This is not an MMIO
sandbox: expressions and pretty printers can still read peripheral addresses.
Do not use this bridge for the LCD registers at 0xc0000000/0xc0000002. Keep using
OpenOCD Tcl RPC with the required access width, clearing AP0 CSW bit 0x08000000
around those LCD accesses and restoring its original value in a finally block.
RAM inspection needs coherent access; do not apply the LCD setting to RAM.

Visual Studio retains connection ownership. Never detach, reconnect, reset or
flash merely to recover the bridge. The current `gdb-detach` handler resumes the
CPU, so ending Visual Studio debugging can destroy a preserved halted failure.
Coordinate execution with the user and any other investigation. Avoid simultaneous
IDE and bridge control operations: GDB dispatch is serialized, but a multi-command
MIEngine interaction is not locked against bridge requests.

Read-only command capture explicitly selects the console interpreter to return
plain text. Breakpoint changes and execution use uncaptured commands so native MI
notifications reach MIEngine. Tests proved that `gdb.execute(..., to_string=True)`
would instead capture breakpoint notifications, and Python `execute_mi` breakpoint
insertion did not emit a creation notification in this installed GDB.

Do not assume the Visual Studio Breakpoints window mirrors bridge-owned entries,
or that explicit GDB selection updates the IDE's selected thread/frame. Manage
bridge-owned entries through the client and IDE-owned entries through Visual
Studio. Native notifications are necessary but do not prove complete IDE state
synchronization. Hardware breakpoint/watchpoint resources remain target-limited.

### Deadlines, cancellation and recovery

Each submission has a unique ID. Requests are serialized in arrival order and
executed through `gdb.post_event`; transport threads do not invoke other GDB APIs.
Socket I/O and completion waits happen in transport workers. The main GDB thread
only signals a nonblocking socketpair when transport shutdown is requested.

The client waits at most `--timeout` seconds (default 5, maximum 30) for the
exchange. `--queue-seconds` separately bounds the time before execution starts
(default 5, maximum 30). Once GDB begins an operation, that deadline cannot stop
it. Long symbol loading or remote operations may therefore outlive a client wait.

Replies carry `id` and `state`: `queued`, `executing`, `completed`, `failed`,
`canceled`, `expired`, `rejected` or `unknown`. Only `canceled` and `expired`
guarantee an accepted operation did not start. `failed` may follow partial effects
from a state-changing command. A disconnected client or an unknown result is not
proof that an operation did not execute. Never automatically retry a mutation.

```powershell
& $gdbPython $gdbClient --endpoint $gdbEndpoint --timeout 15 result REQUEST_ID
& $gdbPython $gdbClient --endpoint $gdbEndpoint cancel REQUEST_ID
```

`result` waits for the existing request; it never resubmits it. `cancel` only
cancels a queued request. An executing request returns `canceled:false` and keeps
running. Duplicate submissions are rejected even after completion. Exit code 0
means completed, 2 means queued/executing/unknown, and 1 means another non-success
state. The in-memory journal retains up to 1024 requests without eviction; when
full, retrieve needed results and stop/reload. Reload loses the journal and creates
a new session identity. A transport ping proves reachability, not GDB responsiveness.

Loading the helper again while active is idempotent. To stop it:

```powershell
& $gdbPython $gdbClient --endpoint $gdbEndpoint request shutdown
```

If the client connection is broken, use Visual Studio instead:

```text
Debug.MIDebugExec pados-bridge stop
Debug.MIDebugExec pados-bridge status
```

Shutdown reports acceptance first, then workers retire, the listener closes, and
the descriptor is removed. Status must report `stopped:true` before sourcing the
helper again. Socket reads/writes have bounded waits, so an incomplete client
cannot leave a worker alive indefinitely. GDB never waits for those workers.
Bridge-created breakpoints survive transport shutdown/reload in the same GDB;
their ownership is retained. Delete unwanted bridge entries explicitly.

If GDB exits abruptly, discovery can show an unreachable stale descriptor. It is
never automatically chosen. Verify the recorded GDB process has exited before
removing that specific file. Startup refuses to overwrite an existing descriptor
for its PID. Do not recover by killing or detaching the live debugging session.
OpenOCD Tcl RPC at 127.0.0.1:6666 remains available for coordinated low-level work;
requests and responses end with byte 0x1a and require no `monitor` prefix.

### Optional startup for future sessions

After the existing unwinder command in `launch.vs.json`, an optional setup command is:

```json
{
  "text": "source ${workspaceRoot}/PadOS/Tools/gdb_pados_bridge.py",
  "ignoreFailures": false
}
```

The current launch configuration is unchanged. Adding this command starts the
transport only; all existing launch/attach and target execution behavior still
belongs to Visual Studio's configuration. Inspect `pados-bridge status` for
asynchronous transport-startup errors even when `source` itself succeeds.

### Validation

Run host tests and the optional unconnected GDB/MI integration test:

```powershell
$env:PATH = 'C:\msys64\mingw64\bin;' + $env:PATH
$env:PYTHONDONTWRITEBYTECODE = '1'
$env:PADOS_TEST_GDB = 'C:\PadOSToolchain\bin\arm-unknown-pados-eabi-gdb.exe'
& C:\msys64\mingw64\bin\python3.exe -m unittest discover -s PadOS/Tools -p test_gdb_pados_bridge.py -v
```

Verified on 2026-10-13: all 11 tests passed, covering dispatch ordering/thread
affinity, authentication, queued cancellation and expiry, in-flight cancellation
refusal, lost-client result retrieval, duplicate rejection, errors, shutdown and
descriptor cleanup. The unconnected GDB test exercised symbolic expressions and
types, native MI breakpoint creation/modification/deletion notifications,
idempotent loading and shutdown/restart. It never connected to OpenOCD.

Live inspection through Visual Studio's existing GDB 18.1 returned the appserver's
`RA8875GfxDriver::WaitBTE` stack, frame locals, registers and disassembly. A type
size expression returned 0x1f0; its initial bounded wait expired while executing,
and retrieving the same request ID later returned the completed result. GDB's
selected thread was restored to 80 after scoped inspection of thread 61, and
Visual Studio's Command Window remained functional, reporting frame 0 and all
three original call/write permissions restored. The Windows sandbox was denied
access to the endpoint descriptor. No target execution or LCD MMIO was changed.
Repeated loading and a complete transport shutdown/restart also passed in that
same live GDB process, with a new session identity and successful inspection
after restart. The root `cmake --build --preset STM32H7-Debug` completed successfully
with `ninja: no work to do`. Matching pre-build ELFs were preserved locally under
`.vs/gdb-bridge-validation/elf-before-build` before that verification.

Live breakpoint/watchpoint hits, continue/halt, instruction/source stepping and
their MIEngine UI synchronization remain unverified pending an agreed execution
interval. Do not infer those results from the offline breakpoint tests.
