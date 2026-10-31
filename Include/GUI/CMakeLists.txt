# This file is part of PadOS.
#
# Copyright (c) 2020 Kurt Skauen
#
# SPDX-License-Identifier: Apache-2.0
###############################################################################

target_sources(PadOS_GUI PRIVATE
	Bitmap.h
	Color.h
	Desktop.h
	Font.h
	FontDescription.h
	GUIDefines.h
	GUIEvent.h
	KeyboardLayout.h
	KeyboardView.h
	LayoutNode.h
	MouseCursor.h
	Region.h
	StyledView.h
	View.h
	ViewBase.h
	ViewFactory.h
	ViewFactoryContext.h
	ViewScroller.h
	Window.h
)

add_subdirectory(Dialogs)
add_subdirectory(Style)
add_subdirectory(Styles)
add_subdirectory(Widgets)
