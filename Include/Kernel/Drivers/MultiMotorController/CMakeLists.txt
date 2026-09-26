# This file is part of PadOS.
#
# Copyright (c) 2025 Kurt Skauen
#
# SPDX-License-Identifier: Apache-2.0
###############################################################################

target_sources(PadOS_Drivers_MultiMotor PRIVATE
	MultiMotorInode.h
	StepperDriver.h
	TMC2209IODriver.h
	TMC2209Driver.h
	TMC2209Registers.h
)
