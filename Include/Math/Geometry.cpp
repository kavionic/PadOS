// This file is part of PadOS.
//
// Copyright (C) 2021 Kurt Skauen <http://kavionic.com/>
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
// Created: 16.05.2021 15:15

#include <algorithm>

#include <Math/Geometry.h>

namespace PMath
{


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PPoint LineLineIntersection(const PLineSegment& line1, const PLineSegment& line2)
{
    const float deltaYL1 = line1.p2.y - line1.p1.y;
    const float deltaXL1 = line1.p1.x - line1.p2.x;

    const float deltaYL2 = line2.p2.y - line2.p1.y;
    const float deltaXL2 = line2.p1.x - line2.p2.x;

    const float determinant = deltaYL1 * deltaXL2 - deltaYL2 * deltaXL1;

    if (determinant == 0.0f) { // Lines are parallel.
        return PPoint(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
    }

    const float c1 = deltaYL1 * line1.p1.x + deltaXL1 * line1.p1.y;
    const float c2 = deltaYL2 * line2.p1.x + deltaXL2 * line2.p1.y;

    PPoint result;
    result.x = (deltaXL2 * c1 - deltaXL1 * c2) / determinant;
    result.y = (deltaYL1 * c2 - deltaYL2 * c1) / determinant;
    return result;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

float PointToSegmentDistance(const PLineSegment& line, const PPoint& point)
{
    return std::sqrt(PointToSegmentDistanceSqr(line, point));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

float PointToSegmentDistanceSqr(const PLineSegment& line, const PPoint& point)
{
    const PPoint lineDirection = line.p2 - line.p1;
    const PPoint pointDirection = point - line.p1;

    const float lineLengthSqr = lineDirection.LengthSqr();

    if (lineLengthSqr == 0.0f) {
        return (point - line.p1).LengthSqr();
    }

    // Project pointDirection onto lineDirection, calculate normalized position along line.
    const float distFromLineStart = std::clamp((pointDirection.x * lineDirection.x + pointDirection.y * lineDirection.y) / lineLengthSqr, 0.0f, 1.0f);

    // Calculate closest point along line
    const PPoint closest = line.p1 + lineDirection * distFromLineStart;

    return (point - closest).LengthSqr();
}

} // namespace PMath
