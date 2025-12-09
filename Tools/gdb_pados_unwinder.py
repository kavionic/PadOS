import gdb
import gdb.unwinder

TRAMPOLINE_NAME     = "kernel::syscall_trampoline_entry"
THREAD_ENTRY_NAME   = "kernel::thread_entry_point"
THREAD_VAR_NAME     = "gk_CurrentThread"

import gdb


def get_function_range_for_pc(pc):
    """Return (start, end) for the function containing PC, or None."""
    fblk = gdb.block_for_pc(pc)
    if fblk is None:
        return None
    return int(fblk.start), int(fblk.end)

def get_function_range(symbol):
    return get_function_range_for_pc(int(symbol.value().address) & ~1)

class PadosSyscallUnwinder(gdb.unwinder.Unwinder):
    def __init__(self):
        super().__init__("pados-syscall")
        # Negative = run *after* normal unwinders
        self.priority = -100
        self._tcb_ptr = None
        self._pc_type = gdb.lookup_type("unsigned int")  # 32-bit

    def __call__(self, pending_frame):
        # Resolve trampoline address once.
        if self._tcb_ptr is None:
            tcb_sym, _ = gdb.lookup_symbol(THREAD_VAR_NAME)
        
            if tcb_sym is None:
                return None

            self._tcb_ptr = tcb_sym.value()
            if self._tcb_ptr is None or self._tcb_ptr.is_optimized_out:
                self._tcb_ptr = None
                return None

            symbol, _ = gdb.lookup_symbol(TRAMPOLINE_NAME)
            if symbol is None:
                return None

            self._tramp_start, self._tramp_end = get_function_range(symbol)

        # Current frame’s PC (this is what we’re unwinding *from*).
        cur_pc = int(pending_frame.read_register("pc")) & ~1
        if not (self._tramp_start <= cur_pc < self._tramp_end):
            # Not the syscall trampoline frame – let other unwinders handle it.
            return None

        # Current frame’s SP (kernel/trampoline frame id).
        cur_sp = int(pending_frame.read_register("sp"))

        # Look up current thread control block.
        tcb = self._tcb_ptr.dereference()

        # m_SyscallReturn contains the user return address with the nPRIV bit
        # stuffed into bit 0. Put back the Thumb bit.
        user_pc = int(tcb["m_SyscallReturn"]) | 1  # force Thumb = 1

#        gdb.write(
#            f"[pados] unwinder: pc=0x{cur_pc:x}, tramp=0x{self._tramp_start:x}, "
#            f"cur_sp=0x{cur_sp:x}, user_pc=0x{user_pc:x}\n"
#        )

        # frame_id describes the *trampoline* frame we are unwinding.
        frame_id = gdb.unwinder.FrameId(cur_sp, cur_pc)
        ui = pending_frame.create_unwind_info(frame_id)

        # Now describe the *previous* (user) frame’s registers:
        pc_val = gdb.Value(user_pc).cast(self._pc_type)
        sp_val = gdb.Value(cur_sp).cast(self._pc_type)

        ui.add_saved_register("pc", pc_val)
        ui.add_saved_register("sp", sp_val)

        # Pass through Cortex-M special/pseudo regs unchanged.
        # 25 = xpsr, 91 = psp, 92 = msp
        for regnum in (25, 91, 92):
            ui.add_saved_register(regnum, pending_frame.read_register(regnum))

        return ui


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
