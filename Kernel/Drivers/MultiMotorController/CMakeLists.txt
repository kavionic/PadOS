# This file is part of PadOS.
#
# Copyright (c) 2025 Kurt Skauen
#
# SPDX-License-Identifier: Apache-2.0
###############################################################################

target_sources(PadOS_Drivers_MultiMotor PRIVATE
	MultiMotorInode.cpp
	StepperDriver.cpp
	TMC2209IODriver.cpp
	TMC2209Driver.cpp
)
