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
// Created: 12.11.2017 16:51:16

#pragma once

#include "Math/Point.h"


class PLineSegment
{
public:
  PLineSegment() {}
  PLineSegment(const PLineSegment& line) : p1(line.p1), p2(line.p2) {}
  PLineSegment(const PPoint& inP1, const PPoint& inP2) : p1(inP1), p2(inP2) {}
  
  PPoint p1;
  PPoint p2;      
};

class PILineSegment
{
public:
  PILineSegment() {}
  PILineSegment(const PILineSegment& line) : p1(line.p1), p2(line.p2) {}
  PILineSegment(const PIPoint& inP1, const PIPoint& inP2) : p1(inP1), p2(inP2) {}
  
  PIPoint p1;
  PIPoint p2;      
};
