// This file is part of PadOS.
//
// Copyright (C) 2020 Kurt Skauen <http://kavionic.com/>
//
// PadOS is free software : you can redistribute it and / or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// PadOS is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with PadOS. If not, see <http://www.gnu.org/licenses/>.
///////////////////////////////////////////////////////////////////////////////
// Created: 28.06.2020 12:56

#pragma once

#include <GUI/Widgets/Control.h>

namespace os
{

class ButtonGroup;

class ButtonBase : public Control
{
public:
	ButtonBase(const String& name, Ptr<View> parent = nullptr, uint32_t flags = 0);
	ButtonBase(ViewFactoryContext* context, Ptr<View> parent, const pugi::xml_node& xmlData, Alignment defaultLabelAlignment);
	~ButtonBase();

	static Ptr<ButtonGroup> FindButtonGroup(Ptr<View> root, const String& name);

	virtual void AllAttachedToScreen() override { Invalidate(); }

	virtual bool OnMouseDown(MouseButton_e button, const Point& position, const MotionEvent& event) override;
	virtual bool OnMouseUp(MouseButton_e button, const Point& position, const MotionEvent& event) override;
	virtual bool OnMouseMove(MouseButton_e button, const Point& position, const MotionEvent& event) override;

	void SetCheckable(bool value) { m_CanBeCheked = value; }
	bool IsCheckable() const { return m_CanBeCheked; }

	void SetChecked(bool isChecked);
	bool IsChecked() const { return m_IsChecked; }

	virtual void OnPressedStateChanged(bool isPressed) { Invalidate(); Flush(); }
	virtual void OnCheckedStateChanged(bool isChecked) { Invalidate(); Flush(); }

	Ptr<ButtonGroup> GetButtonGroup() const;

	Signal<void, MouseButton_e, ButtonBase*>	SignalActivated;
	Signal<void, bool, Ptr<ButtonBase>>			SignalToggled;

protected:
	void SetPressedState(bool isPressed);
	bool GetPressedState() const { return m_IsPressed; }


private:
	friend class ButtonGroup;

	void SetButtonGroup(Ptr<ButtonGroup> group);
	MouseButton_e			m_HitButton = MouseButton_e::None;
	Ptr<ButtonGroup>		m_ButtonGroup;
	bool					m_CanBeCheked = false;
	bool					m_IsPressed = false;
	bool					m_IsChecked = false;


	ButtonBase(const ButtonBase&) = delete;
	ButtonBase& operator=(const ButtonBase&) = delete;
};


} // namespace


