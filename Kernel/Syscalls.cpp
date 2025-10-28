
#include <Kernel/Kernel.h>
#include <Kernel/Scheduler.h>
#include <Kernel/Syscalls.h>
#include <Utils/Utils.h>

using namespace kernel;

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
    SYS_PTR(sbrk),
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
    SYS_PTR(thread_local_create_key),
    SYS_PTR(thread_local_delete_key),
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
    SYS_PTR(delete_handle)
};

static_assert(ARRAY_COUNT(gk_SyscallTable) == SYS_COUNT);

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

    if (syscallNum >= ARRAY_COUNT(gk_SyscallTable)) [[unlikely]]
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
