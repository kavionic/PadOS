// This file is part of PadOS.
//
// Copyright (C) 2017-2018 Kurt Skauen <http://kavionic.com/>
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
// Created: 18/06/19 23:44:30

#pragma once

#include <map>

#include "Ptr/Ptr.h"

class KNodeMonitorNode;

namespace kernel
{

class KIOContext
{
public:
    KIOContext();
    ~KIOContext();
    
    bool AddNodeMonitor(Ptr<KNodeMonitorNode> node);

private:
//    std::map<int, Ptr<KNodeMonitorNode>> m_NodeMonitorMap;
    
    KIOContext(const KIOContext&) = delete;
    KIOContext& operator=(const KIOContext&) = delete;
};

} // namespace
