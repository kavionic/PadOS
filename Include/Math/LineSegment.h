// This file is part of PadOS.
//
// Copyright (c) 2017-2018 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
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
