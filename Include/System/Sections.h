// This file is part of PadOS.
//
// Copyright (c) 2022 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 10.05.2022 18:30

#pragma once

#ifndef IFLASHC
#define IFLASHC __attribute__((section(".itext")))
#endif

#ifndef IFLASHD
#define IFLASHD __attribute__((section(".irodata")))
#endif

#ifndef SECTION_DEVICE_DESCRIPTORS
#define SECTION_DEVICE_DESCRIPTORS __attribute__((section(".drvdesc"), used))
#endif

#ifdef PADOS_MODULE_USER_SPACE
#ifndef SECTION_KERNEL_IMAGE_DEFINITION
#define SECTION_KERNEL_IMAGE_DEFINITION __attribute__((section(".krnimgdesc"), used))
#endif
#endif // PADOS_MODULE_USER_SPACE

#ifndef SECTION_FIRNWARE_IMAGE_DEFINITION
#define SECTION_FIRNWARE_IMAGE_DEFINITION __attribute__((section(".fwimgdesc"), used))
#endif
