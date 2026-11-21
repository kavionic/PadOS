# This file is part of PadOS.
#
# Copyright (c) 2026 Kurt Skauen
#
# SPDX-License-Identifier: Apache-2.0
###############################################################################

target_sources(PadOS_GUI PRIVATE
	SkinnableGUIStyle.h
	DefaultGUIStyle.h
	Default/ViewRendererButtonDefault.h
	Default/ViewRendererButtonDefaultBase.h
	Default/ViewRendererCheckBoxDefault.h
	Default/ViewRendererDropdownMenuPopupFrameDefault.h
	Default/ViewRendererDropdownMenuPopupDefault.h
	Default/ViewRendererDropdownMenuDefault.h
	Default/ViewRendererGroupViewDefault.h
	Default/ViewRendererProgressBarDefault.h
	Default/ViewRendererRadioButtonDefault.h
	Default/ViewRendererScrollBarDefault.h
	Default/ViewRendererSliderDefault.h
	Default/ViewRendererTabViewDefault.h
	Skinned/ViewRendererButtonSkinnedBase.h
	Skinned/ViewRendererButtonSkinned.h
	Skinned/ViewRendererCheckBoxSkinned.h
	Skinned/ViewRendererRadioButtonSkinned.h
	Skinned/ViewRendererDropdownMenuPopupFrameSkinned.h
	Skinned/ViewRendererDropdownMenuPopupSkinned.h
	Skinned/ViewRendererDropdownMenuSkinned.h
	Skinned/ViewRendererGroupViewSkinned.h
	Skinned/ViewRendererProgressBarSkinned.h
	Skinned/ViewRendererScrollBarSkinned.h
	Skinned/SkinnedImage.h
	Skinned/ViewRendererSliderSkinned.h
	Skinned/ViewRendererTabViewSkinned.h
)
