// This file is part of PadOS.
//
// Copyright (C) 2025-2026 Kurt Skauen <http://kavionic.com/>
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


#include <Kernel/Kernel.h>
#include <Kernel/Scheduler.h>
#include <Kernel/Syscalls.h>
#include <Utils/Utils.h>


namespace kernel
{

PErrorCode sys_unimplemented() { return PErrorCode::NotImplemented; }



template<int A, int B> struct sys_check_eq { static_assert(A == B, "SYS index mismatch"); };
#define SYS_CHECK_EQ(a,b) (void)sizeof(sys_check_eq<(a),(b)>)

#define SYS_PTR(name) (SYS_CHECK_EQ((SYS_##name), (int)(__COUNTER__ - SYS_COUNTER_START - 1) ), reinterpret_cast<void*>(&sys_##name))

static constexpr int SYS_COUNTER_START = __COUNTER__;

static const void* const gk_SyscallTable[] =
{
    SYS_PTR(open),
    SYS_PTR(openat),
    SYS_PTR(reopen_file),
    SYS_PTR(close),
    SYS_PTR(fcntl),
    SYS_PTR(dup),
    SYS_PTR(dup2),
    SYS_PTR(rename),
    SYS_PTR(fstat),
    SYS_PTR(stat),
    SYS_PTR(write_stat),
    SYS_PTR(isatty),
    SYS_PTR(seek),
    SYS_PTR(read),
    SYS_PTR(read_pos),
    SYS_PTR(readv),
    SYS_PTR(readv_pos),
    SYS_PTR(write),
    SYS_PTR(write_pos),
    SYS_PTR(writev),
    SYS_PTR(writev_pos),
    SYS_PTR(device_control),
    SYS_PTR(create_directory),
    SYS_PTR(read_directory),
    SYS_PTR(rewind_directory),
    SYS_PTR(remove_directory),
    SYS_PTR(unlink_file),
    SYS_PTR(readlink),
    SYS_PTR(symlink),
    SYS_PTR(get_directory_path),
    SYS_PTR(chdir),
    SYS_PTR(getcwd),
    SYS_PTR(fsync),
    SYS_PTR(mount),
    SYS_PTR(get_monotonic_time_ns),
    SYS_PTR(get_monotonic_time_hires_ns),
    SYS_PTR(get_real_time_ns),
    SYS_PTR(get_real_time_hires_ns),
    SYS_PTR(set_real_time_ns),
    SYS_PTR(get_clock_time_offset_ns),
    SYS_PTR(get_clock_time_ns),
    SYS_PTR(get_clock_time_hires_ns),
    SYS_PTR(get_idle_time_ns),
    SYS_PTR(get_clock_resolution_ns),
    SYS_PTR(set_clock_resolution_ns),
    SYS_PTR(thread_attribs_init),
    SYS_PTR(thread_spawn),
    SYS_PTR(thread_exit),
    SYS_PTR(thread_detach),
    SYS_PTR(thread_join),
    SYS_PTR(get_thread_id),
    SYS_PTR(thread_set_priority),
    SYS_PTR(thread_get_priority),
    SYS_PTR(get_thread_info),
    SYS_PTR(get_next_thread_info),
    SYS_PTR(snooze_ns),
    SYS_PTR(snooze_until_ns),
    SYS_PTR(yield),
    SYS_PTR(thread_kill),
    SYS_PTR(getpid),
    SYS_PTR(kill),
    SYS_PTR(get_dirty_disk_cache_blocks),
    SYS_PTR(exit),
    SYS_PTR(sysconf),
    SYS_PTR(semaphore_create),
    SYS_PTR(semaphore_duplicate),
    SYS_PTR(semaphore_delete),
    SYS_PTR(semaphore_create_public),
    SYS_PTR(semaphore_unlink_public),
    SYS_PTR(semaphore_acquire),
    SYS_PTR(semaphore_acquire_timeout_ns),
    SYS_PTR(semaphore_acquire_deadline_ns),
    SYS_PTR(semaphore_acquire_clock_ns),
    SYS_PTR(semaphore_try_acquire),
    SYS_PTR(semaphore_release),
    SYS_PTR(semaphore_get_count),
    SYS_PTR(mutex_create),
    SYS_PTR(mutex_duplicate),
    SYS_PTR(mutex_delete),
    SYS_PTR(mutex_lock),
    SYS_PTR(mutex_lock_timeout_ns),
    SYS_PTR(mutex_lock_deadline_ns),
    SYS_PTR(mutex_lock_clock_ns),
    SYS_PTR(mutex_try_lock),
    SYS_PTR(mutex_unlock),
    SYS_PTR(mutex_lock_shared),
    SYS_PTR(mutex_lock_shared_timeout_ns),
    SYS_PTR(mutex_lock_shared_deadline_ns),
    SYS_PTR(mutex_lock_shared_clock_ns),
    SYS_PTR(mutex_try_lock_shared),
    SYS_PTR(mutex_islocked),
    SYS_PTR(condition_var_create),
    SYS_PTR(condition_var_delete),
    SYS_PTR(condition_var_wait),
    SYS_PTR(condition_var_wait_timeout_ns),
    SYS_PTR(condition_var_wait_deadline_ns),
    SYS_PTR(condition_var_wait_clock_ns),
    SYS_PTR(condition_var_wakeup),
    SYS_PTR(condition_var_wakeup_all),
    SYS_PTR(reboot),
    SYS_PTR(object_wait_group_create),
    SYS_PTR(object_wait_group_delete),
    SYS_PTR(object_wait_group_add_object),
    SYS_PTR(object_wait_group_remove_object),
    SYS_PTR(object_wait_group_add_file),
    SYS_PTR(object_wait_group_remove_file),
    SYS_PTR(object_wait_group_clear),
    SYS_PTR(object_wait_group_wait),
    SYS_PTR(object_wait_group_wait_timeout_ns),
    SYS_PTR(object_wait_group_wait_deadline_ns),
    SYS_PTR(message_port_create),
    SYS_PTR(message_port_duplicate),
    SYS_PTR(message_port_delete),
    SYS_PTR(message_port_send),
    SYS_PTR(message_port_send_timeout_ns),
    SYS_PTR(message_port_send_deadline_ns),
    SYS_PTR(message_port_receive),
    SYS_PTR(message_port_receive_timeout_ns),
    SYS_PTR(message_port_receive_deadline_ns),
    SYS_PTR(get_total_irq_time_ns),
    SYS_PTR(duplicate_handle),
    SYS_PTR(delete_handle),
    SYS_PTR(is_debugger_attached),
    SYS_PTR(digital_pin_set_direction),
    SYS_PTR(digital_pin_set_drive_strength),
    SYS_PTR(digital_pin_set_pull_mode),
    SYS_PTR(digital_pin_set_peripheral_mux),
    SYS_PTR(digital_pin_read),
    SYS_PTR(digital_pin_write),
    SYS_PTR(write_backup_register),
    SYS_PTR(read_backup_register),
    SYS_PTR(beep_seconds),
    SYS_PTR(system_log_register_category),
    SYS_PTR(system_log_set_category_minimum_severity),
    SYS_PTR(system_log_is_category_active),
    SYS_PTR(system_log_get_category_channel),
    SYS_PTR(system_log_get_severity_name),
    SYS_PTR(system_log_get_category_name),
    SYS_PTR(system_log_get_category_display_name),
    SYS_PTR(system_log_add_message),
    SYS_PTR(add_serial_command_handler),
    SYS_PTR(serial_command_send_data),
    SYS_PTR(spawn_execve),
    SYS_PTR(sigaction),
    SYS_PTR(thread_sigqueue),
    SYS_PTR(thread_sigmask),
    SYS_PTR(raise),
    SYS_PTR(signal),
    SYS_PTR(sigsuspend)
};

static_assert(ARRAY_COUNT(gk_SyscallTable) == SYS_COUNT);

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

extern "C" uint32_t syscall_return()
{
    const KThreadCB& thread = *gk_CurrentThread;
    if (thread.HasUnblockedPendingSignals()) {
        kforce_process_signals();
    }
    return thread.m_SyscallReturn;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

extern "C" __attribute__((naked)) void syscall_trampoline_entry(void)
{
    __asm volatile (
        "   blx     r12\n"              // Call syscall.
        "   push    {r0, r1}\n"         // Preserve the syscall return value.
        "   bl      syscall_return\n"
        "   mov     r2, r0\n"           // syscall_return() returns the caller address + privilege level.
        "   pop     {r0, r1}\n"         // Restore the syscall return value.
        "   mrs     r12, CONTROL\n"
        "   bfi     r12, r2, #0, #1\n"  // Bit 0 of the return address contain the privilege level.
        "   msr     CONTROL, r12\n"
        "   isb\n"                      // Flush instruction pipeline.
        "   orr     r2, r2, #1\n"       // Set the thumb flag.
        "   bx      r2\n"               // Return directly to caller.
    );
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

extern "C" void SetupSystemCall(KExceptionStackFrame* frame, uint32_t syscallNum, uint32_t prevControlReg)
{
    // Store the return address with the thumb flag replaced by the privilege level.
    gk_CurrentThread->m_SyscallReturn = (frame->LR & ~0x01) | (prevControlReg & 0x01);

    frame->R12 = reinterpret_cast<uintptr_t>(gk_SyscallTable[syscallNum]);
    frame->PC  = reinterpret_cast<uintptr_t>(syscall_trampoline_entry); // Run trampoline next.
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

extern "C" __attribute__((naked)) void SVCall_Handler(void)
{
    __asm volatile (
    "   tst     lr, #4\n"       // EXC_RETURN bit2: 0=MSP, 1=PSP
    "   ite     eq\n"
    "   mrseq   r0, msp\n"      // r0 = exception frame (arg 0)
    "   mrsne   r0, psp\n"
    "   ldr     r1, [r0, %0]\n" // r1 = syscall number (arg 1)
    "   cmp     r1, %1\n"
    "   bhi     .invalid_syscall\n"
    ""
    "   mrs     r2, CONTROL\n"  // r2 = CONTROL (arg 2)
    "   mov     r3, r2\n"
    "   bfc     r3, #0, #1\n"   // Clear nPRIV (bit 0).
    "   msr     CONTROL, r3\n"
    "   isb\n"
    "   b      SetupSystemCall\n"   // SetupSystemCall(stackPtr, SyscallNum, prevCONTROL)
    ""
    ".invalid_syscall:\n"
    "   cmp     r1, %2\n"           // SYS_sigreturn
    "   beq     .sigreturn\n"
    "   cmp     r1, %3\n"           // SYS_process_signals
    "   beq     .process_signals\n"
    "   ldr     r1, =%4\n"          // ENOSYS
    "   str     r1, [r0, %5]\n"     // frame -> R0 (return ENOSYS).
    "   ldr     r1, [r0, %6]\n"     // Read frame -> LR.
    "   str     r1, [r0, %7]\n"     // Write frame -> PC.
    "   bx      lr\n"
    ""
    ".sigreturn:\n" // Used by the signal-return trampoline to restore normal thread context.
    "   ldr     r2, [r0, #28]\n"    // Stacked xPSR (basic frame) : 7*4
    "   lsrs    r2, r2, #9\n"       // Shift ALIGN bit to bit-0
    "   ands    r2, r2, #1\n"
    "   lsls    r2, r2, #2\n"       // r2 = 0 or 4
    "   add     r0, r0, r2\n"       // Skip padding if present.
    "   tst     lr, #0x10\n"        // Test bit-4 in EXEC_RETURN to check if the thread use the FPU context.
    "   ite     eq\n"
    "   addeq   r0, %9\n"           // Remove exception frame with FPU registers.
    "   addne   r0, %8\n"           // Remove exception frame without FPU registers.
    "   bl      ksigreturn\n"
        ASM_LOAD_SCHED_CONTEXT(r0)
    "   msr     psp, r0\n"
    "   bx      lr\n"
    ""
    ".process_signals:\n"   // Used to force synchronous signal handling after regular syscalls.
        ASM_STORE_SCHED_CONTEXT(r0)
    "   mrs     r1, CONTROL\n"
    "   and     r1, #1\n"   // r1=nPRIV
    "   mov     r4, r0\n"
    "   bl      kprocess_pending_signals\n"  // kprocess_pending_signals(currentStack[r0], userMode[r1])
    "   cmp     r0, r4\n"
    "   beq     .no_signal_added\n"
    "   mrs     r2, CONTROL\n"
    "   orr     r2, #1\n"       // Set nPRIV (bit 0).
    "   msr     CONTROL, r2\n"  // Drop privilege before entering signal handler.
    "   isb\n"
    ".no_signal_added:\n"
        ASM_LOAD_SCHED_CONTEXT(r0)
    "   msr     psp, r0\n"
    "   bx      lr\n"
    ""
    ::  "i"(offsetof(KExceptionStackFrame, R12)),       // %0
        "i"(ARRAY_COUNT(gk_SyscallTable) - 1),          // %1
        "i"(SYS_sigreturn),                             // %2
        "i"(SYS_process_signals),                       // %3
        "i"(ENOSYS),                                    // %4
        "i"(offsetof(KExceptionStackFrame, R0)),        // %5
        "i"(offsetof(KExceptionStackFrame, LR)),        // %6
        "i"(offsetof(KExceptionStackFrame, PC)),        // %7
        "i"(sizeof(KExceptionStackFrame)),              // %8
        "i"(sizeof(KExceptionStackFrameFPU))            // %9
    );
}


} // namespace kernel
