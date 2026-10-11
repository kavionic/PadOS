# This file is part of PadOS.
#
# Copyright (c) 2020 Kurt Skauen
#
# SPDX-License-Identifier: Apache-2.0
###############################################################################

target_sources(PadOS_GUI PRIVATE
	Application.cpp
	Bitmap.cpp
	Color.cpp
	Font.cpp
	KeyboardView.cpp
	LayoutNode.cpp
	View.cpp
	ViewFactory.cpp
	ViewFactoryContext.cpp
	ViewScroller.cpp
	Window.cpp
)

target_sources(PadOS_System PRIVATE FontMetrics.cpp)

add_subdirectory(Fonts)
add_subdirectory(Dialogs)
add_subdirectory(Widgets)
