# This file is part of PadOS.
#
# Copyright (c) 2020 Kurt Skauen
#
# SPDX-License-Identifier: Apache-2.0
###############################################################################

target_sources(PadOS_GUI PRIVATE
	ViewBrush.cpp
	Brush.cpp
	BrushSharedData.cpp
	Image.cpp
	ImageData.cpp
	Application.cpp
	Bitmap.cpp
	BitmapDecoder.cpp
	BitmapLoader.cpp
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

add_subdirectory(Dialogs)
add_subdirectory(Style)
add_subdirectory(Styles)
add_subdirectory(Widgets)
