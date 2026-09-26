# This file is part of PadOS.
#
# Copyright (c) 2020 Kurt Skauen
#
# SPDX-License-Identifier: Apache-2.0
###############################################################################

target_sources(PadOS_System PRIVATE
  HandleObject.cpp
  NewlibRetargetLocking.cpp
  Time.cpp
)

add_subdirectory(GUI)
add_subdirectory(Math)
add_subdirectory(Process)
add_subdirectory(Ptr)
add_subdirectory(Signals)
add_subdirectory(Storage)
add_subdirectory(SyscallStubs)
add_subdirectory(System)
add_subdirectory(Threads)
add_subdirectory(Utils)
