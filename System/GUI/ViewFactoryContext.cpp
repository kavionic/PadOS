// This file is part of PadOS.
//
// Copyright (c) 2020 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
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
