# This file is part of PadOS.
#
# Copyright (c) 2025 Kurt Skauen
#
# SPDX-License-Identifier: Apache-2.0
###############################################################################

import gdb
import gdb.unwinder

TRAMPOLINE_NAME     = "kernel::syscall_trampoline_entry"
THREAD_ENTRY_NAME   = "kernel::thread_entry_point"
MAX_DEBUGGER_THREADS = 1024 * 1024


def get_function_range_for_pc(pc):
    """Return (start, end) for the function containing PC, or None."""
    fblk = gdb.block_for_pc(pc)
    if fblk is None:
        return None
    return int(fblk.start), int(fblk.end)

def get_function_range(symbol):
    return get_function_range_for_pc(int(symbol.value().address) & ~1)

def read_debugger_list_field(debugger_info, offset_field):
    field_address = int(debugger_info["Threads"]) + int(debugger_info[offset_field])
    word_pointer_type = debugger_info["Version"].type.pointer()
    return int(gdb.Value(field_address).cast(word_pointer_type).dereference())


def get_selected_thread_syscall_return():
    selected_thread = gdb.selected_thread()
    if selected_thread is None:
        return None

    # GDB stores the remote protocol's thread ID in the LWP component.
    thread_id = selected_thread.ptid[1]
    symbol = gdb.lookup_global_symbol("kernel::_kernel_debugger_info")
    if symbol is None:
        return None

    # Use exported list offsets and the ELF's thread and intrusive-node types.
    debugger_info = symbol.value()
    node_address = read_debugger_list_field(debugger_info, "ThreadListFirstOffset")
    node_offset = int(debugger_info["ThreadNodeOffset"])
    thread_pointer_type = debugger_info["CurrentThread"].type.target().unqualified()
    for _ in range(MAX_DEBUGGER_THREADS):
        if node_address <= node_offset or node_address & 3:
            return None
        thread = gdb.Value(node_address - node_offset).cast(thread_pointer_type).dereference()
        if int(thread["m_Handle"]) == thread_id:
            return int(thread["m_SyscallReturn"])
        node_address = int(thread["m_DebuggerListNode"]["m_Next"])
    return None


class PadosSyscallUnwinder(gdb.unwinder.Unwinder):
    def __init__(self):
        super().__init__("pados-syscall")

    def __call__(self, pending_frame):
        symbol = gdb.lookup_global_symbol(TRAMPOLINE_NAME)
        if symbol is None:
            return None
        trampoline_range = get_function_range(symbol)
        if trampoline_range is None:
            return None
        trampoline_start, trampoline_end = trampoline_range

        current_pc = int(pending_frame.read_register("pc")) & ~1
        if not (trampoline_start <= current_pc < trampoline_end):
            return None

        syscall_return = get_selected_thread_syscall_return()
        if syscall_return is None or (syscall_return & ~1) == 0:
            return None

        # m_SyscallReturn stores nPRIV in bit 0; restore the Thumb bit.
        register_type = pending_frame.read_register("pc").type
        user_pc = gdb.Value(syscall_return | 1).cast(register_type)
        current_sp = pending_frame.read_register("sp")
        frame_id = gdb.unwinder.FrameId(current_sp, current_pc)
        unwind_info = pending_frame.create_unwind_info(frame_id)
        unwind_info.add_saved_register("pc", user_pc)
        unwind_info.add_saved_register("sp", current_sp)

        # Preserve the caller's nonvolatile core registers across the trampoline.
        for register_name in [f"r{index}" for index in range(4, 12)] + ["xpsr", "msp", "psp"]:
            unwind_info.add_saved_register(register_name, pending_frame.read_register(register_name))

        # D8-D15 and their S16-S31 aliases are callee-saved. Python unwinders must
        # describe both. Threads that never used the FPU may have no saved context.
        for register_name in [f"d{index}" for index in range(8, 16)] + [f"s{index}" for index in range(16, 32)]:
            try:
                unwind_info.add_saved_register(register_name, pending_frame.read_register(register_name))
            except gdb.error:
                pass

        return unwind_info


class PadosThreadBottomUnwinder(gdb.unwinder.Unwinder):
    def __init__(self):
        super().__init__("pados-thread-bottom")
        self.priority = -50  # run after normal unwinders
        self._entry_addr = None
        self._entry_end  = None

    def __call__(self, pending_frame):
        if self._entry_addr is None:
            sym, _ = gdb.lookup_symbol(THREAD_ENTRY_NAME)
            if sym is None:
                return None
            self._entry_addr, self._entry_end  = get_function_range(sym)

        pc = int(pending_frame.read_register("pc")) & ~1
        if not (self._entry_addr <= pc < self._entry_end):
            return None  # not our frame

        sp = int(pending_frame.read_register("sp"))

        # Tell GDB “this is a frame, but there is *no* caller”.
        frame_id = gdb.unwinder.FrameId(sp, pc)
        ui = pending_frame.create_unwind_info(frame_id)

        # NOTE: we intentionally do *not* call ui.add_saved_register("pc", ...).
        # With no saved PC, GDB stops unwinding here and doesn’t invent junk
        # frames below this one.

        return ui


pspace = gdb.current_progspace()

gdb.unwinder.register_unwinder(pspace, PadosSyscallUnwinder(), replace=True)
gdb.unwinder.register_unwinder(pspace, PadosThreadBottomUnwinder(), replace=True)
