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
// Created: 30.06.2020 17:49

#include <GUI/ViewFactoryContext.h>
#include <GUI/Widgets/ButtonGroup.h>


Ptr<PButtonGroup> PViewFactoryContext::GetButtonGroup(const PString& name)
{
	Ptr<PButtonGroup> group;
	auto i = m_ButtonGroups.find(name);
	if (i != m_ButtonGroups.end()) {
		group = i->second;
	} else {
		group = ptr_new<PButtonGroup>(name);
		m_ButtonGroups[name] = group;
	}
	return group;
}
