# This file is part of PadOS.
#
# Copyright (c) 2020 Kurt Skauen
#
# SPDX-License-Identifier: Apache-2.0
###############################################################################

target_sources(PadOS_System PRIVATE
	Acceleration.cpp
	Acceleration.h
	Geometry.cpp
	Geometry.h
	LineSegment.h
	Misc.h
	PIDController.h
	Point.h
	Rect.h
	Vector.h
)
