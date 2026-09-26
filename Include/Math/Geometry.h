// This file is part of PadOS.
//
// Copyright (c) 2021 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 16.05.2021 15:15

#pragma once

#include <Math/LineSegment.h>

namespace PMath
{

PPoint LineLineIntersection(const PLineSegment& line1, const PLineSegment& line2);
float PointToSegmentDistance(const PLineSegment& line, const PPoint& point);
float PointToSegmentDistanceSqr(const PLineSegment& line, const PPoint& point);

} // namespace PMath
