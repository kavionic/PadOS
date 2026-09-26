/* This file is part of PadOS.
 * 
 * Copyright (c) 2020-2024 Kurt Skauen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/dirent.h>
#include <sys/cdefs.h>
#include <stdbool.h>
#include <sys/pados_types.h>
#include <sys/pados_error_codes.h>
#include <sys/pados_syscalls.h>
#include <sys/pados_threads.h>
#include <sys/pados_mutex.h>
#include <PadOS/BootMode.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*TLSDestructor_t)(void*);

#define PEXPAND_SYSCALL(EPILOGUE, RETTYPE, RETTYPE_SYS, FPREFIX, FNAME, SIGNATURE) RETTYPE_SYS sys_##FNAME(PDECL_LIST(SIGNATURE));
#include <PadOS/SyscallDefinitions.h>
#undef PEXPAND_SYSCALL

#define PEXPAND_SYSCALL(EPILOGUE, RETTYPE, RETTYPE_SYS, FPREFIX, FNAME, SIGNATURE) RETTYPE_SYS ksys_##FNAME(PDECL_LIST(SIGNATURE));
#include <PadOS/SyscallDefinitions.h>
#undef PEXPAND_SYSCALL

#ifdef __cplusplus
}
#endif
