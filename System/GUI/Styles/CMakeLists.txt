# This file is part of PadOS.
#
# Copyright (c) 2026 Kurt Skauen
#
# SPDX-License-Identifier: Apache-2.0
###############################################################################

target_sources(PadOS_GUI PRIVATE
	SkinnableGUIStyle.cpp
	DefaultGUIStyle.cpp
	Default/ViewRendererButtonDefault.cpp
	Default/ViewRendererButtonDefaultBase.cpp
	Default/ViewRendererCheckBoxDefault.cpp
	Default/ViewRendererDropdownMenuPopupFrameDefault.cpp
	Default/ViewRendererDropdownMenuPopupDefault.cpp
	Default/ViewRendererDropdownMenuDefault.cpp
	Default/ViewRendererGroupViewDefault.cpp
	Default/ViewRendererProgressBarDefault.cpp
	Default/ViewRendererRadioButtonDefault.cpp
	Default/ViewRendererScrollBarDefault.cpp
	Default/ViewRendererSliderDefault.cpp
	Default/ViewRendererTabViewDefault.cpp
	Skinned/ViewRendererButtonSkinnedBase.cpp
	Skinned/ViewRendererButtonSkinned.cpp
	Skinned/ViewRendererCheckBoxSkinned.cpp
	Skinned/ViewRendererRadioButtonSkinned.cpp
	Skinned/ViewRendererDropdownMenuPopupFrameSkinned.cpp
	Skinned/ViewRendererDropdownMenuPopupSkinned.cpp
	Skinned/ViewRendererDropdownMenuSkinned.cpp
	Skinned/ViewRendererGroupViewSkinned.cpp
	Skinned/ViewRendererProgressBarSkinned.cpp
	Skinned/ViewRendererScrollBarSkinned.cpp
	Skinned/SkinnedImage.cpp
	Skinned/ViewRendererSliderSkinned.cpp
	Skinned/ViewRendererTabViewSkinned.cpp
)
