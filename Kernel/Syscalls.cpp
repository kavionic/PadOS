
#include <Kernel/Kernel.h>
#include <Kernel/Scheduler.h>
#include <Kernel/Syscalls.h>
#include <Utils/Utils.h>

using namespace kernel;

#define SYS_PTR(f) reinterpret_cast<void*>(f)
static const void* const gk_SyscallTable[] =
{
    SYS_PTR(sys_open),
    SYS_PTR(sys_openat),
    SYS_PTR(sys_close),
    SYS_PTR(sys_fcntl),
    SYS_PTR(sys_dup),
    SYS_PTR(sys_dup2),
    SYS_PTR(sys_rename),
    SYS_PTR(sys_fstat),
    SYS_PTR(sys_stat),
    SYS_PTR(sys_write_stat),
    SYS_PTR(sys_isatty),
    SYS_PTR(sys_lseek),
    SYS_PTR(sys_read),
    SYS_PTR(sys_read_pos),
    SYS_PTR(sys_readv),
    SYS_PTR(sys_readv_pos),
    SYS_PTR(sys_write),
    SYS_PTR(sys_write_pos),
    SYS_PTR(sys_writev),
    SYS_PTR(sys_writev_pos),
    SYS_PTR(sys_create_directory),
    SYS_PTR(sys_create_directory_base),
    SYS_PTR(sys_read_directory),
    SYS_PTR(sys_unlink_file),
    SYS_PTR(sys_remove_directory),
    SYS_PTR(sys_readlink),
    SYS_PTR(sys_symlink),
    SYS_PTR(sys_chdir),
    SYS_PTR(sys_getcwd),
    SYS_PTR(sys_get_system_time),
    SYS_PTR(sys_get_system_time_hires),
    SYS_PTR(sys_get_real_time),
    SYS_PTR(sys_set_real_time),
    SYS_PTR(sys_get_clock_time_offset),
    SYS_PTR(sys_get_clock_time),
    SYS_PTR(sys_get_clock_time_hires),
    SYS_PTR(sys_get_clock_resolution),
    SYS_PTR(sys_set_clock_resolution),
    SYS_PTR(sys_thread_attribs_init),
    SYS_PTR(sys_thread_spawn),
    SYS_PTR(sys_thread_exit),
    SYS_PTR(sys_thread_detach),
    SYS_PTR(sys_thread_join),
    SYS_PTR(sys_get_thread_id),
    SYS_PTR(sys_thread_set_priority),
    SYS_PTR(sys_thread_get_priority),
    SYS_PTR(sys_get_thread_info),
    SYS_PTR(sys_get_next_thread_info),
    SYS_PTR(sys_snooze_ns),
    SYS_PTR(sys_yield),
    SYS_PTR(sys_thread_kill),
    SYS_PTR(sys_getpid),
    SYS_PTR(sys_kill),
    SYS_PTR(sys_sbrk),
    SYS_PTR(sys_exit),
    SYS_PTR(sys_sysconf),
    SYS_PTR(sys_semaphore_create),
    SYS_PTR(sys_semaphore_duplicate),
    SYS_PTR(sys_semaphore_delete),
    SYS_PTR(sys_semaphore_create_public),
    SYS_PTR(sys_semaphore_unlink_public),
    SYS_PTR(sys_semaphore_acquire),
    SYS_PTR(sys_semaphore_acquire_timeout_ns),
    SYS_PTR(sys_semaphore_acquire_deadline_ns),
    SYS_PTR(sys_semaphore_acquire_clock_ns),
    SYS_PTR(sys_semaphore_try_acquire),
    SYS_PTR(sys_semaphore_release),
    SYS_PTR(sys_semaphore_get_count),
    SYS_PTR(sys_mutex_create),
    SYS_PTR(sys_mutex_duplicate),
    SYS_PTR(sys_mutex_delete),
    SYS_PTR(sys_mutex_lock),
    SYS_PTR(sys_mutex_lock_timeout_ns),
    SYS_PTR(sys_mutex_lock_deadline_ns),
    SYS_PTR(sys_mutex_lock_clock_ns),
    SYS_PTR(sys_mutex_try_lock),
    SYS_PTR(sys_mutex_unlock),
    SYS_PTR(sys_mutex_lock_shared),
    SYS_PTR(sys_mutex_lock_shared_timeout_ns),
    SYS_PTR(sys_mutex_lock_shared_deadline_ns),
    SYS_PTR(sys_mutex_lock_shared_clock_ns),
    SYS_PTR(sys_mutex_try_lock_shared),
    SYS_PTR(sys_mutex_islocked),
    SYS_PTR(sys_condition_var_create),
    SYS_PTR(sys_condition_var_delete),
    SYS_PTR(sys_condition_var_wait),
    SYS_PTR(sys_condition_var_wait_timeout_ns),
    SYS_PTR(sys_condition_var_wait_deadline_ns),
    SYS_PTR(sys_condition_var_wait_clock_ns),
    SYS_PTR(sys_condition_var_wakeup),
    SYS_PTR(sys_condition_var_wakeup_all),
    SYS_PTR(sys_thread_local_create_key),
    SYS_PTR(sys_thread_local_delete_key),
    SYS_PTR(sys_thread_local_set),
    SYS_PTR(sys_thread_local_get),
    SYS_PTR(sys_get_idle_time),
    SYS_PTR(sys_reboot),
    SYS_PTR(sys_snooze_until)
};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

extern "C" __attribute__((naked)) void syscall_trampoline_entry(void)
{
    __asm volatile (
        "blx    r12\n"              // Call syscall.
        "ldr    r12, =gk_CurrentThread\n"
        "ldr    r12, [r12]\n"
        "ldr    r2, [r12, %0]\n"    // Return address / privilege level
        "mrs    r12, CONTROL\n"
        "bfi    r12, r2, #0, #1\n"  // Bit 0 of the return address contain the privilege level.
        "msr    CONTROL, r12\n"
        "isb\n"                     // Flush instruction pipeline.
        "orr    r2, r2, #1\n"       // Set the thumb flag.
        "bx     r2\n"               // Return directly to caller.
        :: "i"(offsetof(KThreadCB, m_SyscallReturn))
    );
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

extern "C" void SetupSystemCall(KExceptionStackFrame* frame)
{
    uint32_t syscallNum = frame->R12;

    if (__builtin_expect(syscallNum >= ARRAY_COUNT(gk_SyscallTable), 0))
    {
        frame->R0 = ENOSYS;
        return;
    }

    uint32_t prevControl;
    __asm volatile ("mrs %0, CONTROL" : "=r"(prevControl));
    const uint32_t newControl = prevControl & ~1u; // Clear nPRIV (privileged).
    __asm volatile (
        "msr CONTROL, %0 \n"
        "isb\n"
        :: "r"(newControl) : "memory"
    );

    // Store the return address with the thumb flag replaced by the privilege level.
    gk_CurrentThread->m_SyscallReturn = (frame->LR & ~0x01) | (prevControl & 0x01);

    frame->R12 = reinterpret_cast<uintptr_t>(gk_SyscallTable[syscallNum]);
    frame->PC  = reinterpret_cast<uintptr_t>(syscall_trampoline_entry); // Run trampoline next.
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

extern "C" __attribute__((naked)) void SVCall_Handler(void)
{
    __asm volatile (
        "tst    lr, #4\n"   // EXC_RETURN bit2: 0=MSP, 1=PSP
        "ite    eq\n"
        "mrseq  r0, msp\n"  // r0 = exception frame
        "mrsne  r0, psp\n"
        "b      SetupSystemCall\n"
    );
}
