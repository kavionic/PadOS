// This file is part of PadOS.
//
// Copyright (c) 2018 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 08.03.2018 23:02:13

#pragma once


#include <sys/types.h>

#include <System/Types.h>
#include <System/ErrorCodes.h>


#ifdef __cplusplus
extern "C"
#endif
void launch_pados(size_t mainThreadStackSize);


int get_last_error();
void set_last_error(int error);
#ifdef __cplusplus
void set_last_error(PErrorCode error);
#endif
