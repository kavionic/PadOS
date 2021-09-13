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
// Created: 16.05.2021 15:00

#include <stdio.h>
#include <math.h>

#include <Math/Misc.h>
#include <Math/Point.h>
#include <Math/Geometry.h>

#include "Acceleration.h"

using namespace os;


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

float Acceleration::CalculateAccelerationSpeed(float distance, float startSpeed, float acceleration)
{
    return sqrtf(square(startSpeed) + fabsf(distance) * acceleration * 2.0f) - startSpeed;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

float Acceleration::CalcAccelerationDistance(float startSpeed, float resultSpeed, float acceleration)
{
    return (fabsf(square(resultSpeed) - square(startSpeed))) / (acceleration * 2.0f);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

float Acceleration::CalcMaxSpeedPermittingStop(float distance, float startSpeed, float endSpeed, float acceleration)
{
    return (sqrt(fabsf((-(square(startSpeed))) + 2.0f * square(endSpeed) - square(endSpeed) + 4.0f * acceleration * distance)) + startSpeed + endSpeed) * 0.5f;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool Acceleration::CalculateMaxCruiseSpeed(float distance, float startSpeed, float cruiseSpeed, float endSpeed, float acceleration, float& outAccDist, float& outDecDist, float& outMaxCruiseSpeed, float& outMaxEndSpeed)
{
    if (endSpeed > cruiseSpeed) endSpeed = cruiseSpeed;
    float deltaSpeed = fabsf(endSpeed - startSpeed);
    float maxDeltaSpeed = CalculateAccelerationSpeed(distance, startSpeed, acceleration);

    float maxCruiseSpeed;
    if (maxDeltaSpeed >= deltaSpeed)
    {
        bool reversed = false;
        if (startSpeed > endSpeed)
        {
            std::swap(startSpeed, endSpeed);
            reversed = true;
        }
        float maxStartSpeed = endSpeed + maxDeltaSpeed;
        float maxEndSpeed = startSpeed + maxDeltaSpeed;

        float maxSpeed = LineLineIntersection(LineSegment(Point(0.0f, startSpeed), Point(distance, maxEndSpeed)), LineSegment(Point(distance, endSpeed), Point(0.0f, maxStartSpeed))).y;
        maxCruiseSpeed = std::min(cruiseSpeed, maxSpeed);

        outAccDist = CalcAccelerationDistance(startSpeed, maxCruiseSpeed, acceleration);
        outDecDist = CalcAccelerationDistance(maxCruiseSpeed, endSpeed, acceleration);

        outMaxCruiseSpeed = maxCruiseSpeed;
        if (reversed)
        {
            std::swap(outAccDist, outDecDist);
            outMaxEndSpeed = startSpeed;
        }
        else
        {
            outMaxEndSpeed = endSpeed;
        }
        return true;
    }
    else
    {
        if (startSpeed < endSpeed)
        {
            outMaxCruiseSpeed = startSpeed + maxDeltaSpeed;
            outMaxEndSpeed = outMaxCruiseSpeed;
            outAccDist = distance;
            outDecDist = 0.0f;
        }
        else
        {
            outMaxCruiseSpeed = startSpeed;
            outMaxEndSpeed = startSpeed - maxDeltaSpeed;
            outAccDist = 0.0f;
            outDecDist = distance;
        }
        return false;
    }
}

float Acceleration::CalcTravelTime(float distance, float startSpeed, float cruiseSpeed, float endSpeed, float acceleration)
{
    float accDist;
    float decDist;
    float maxCruiseSpeed;
    float maxEndSpeed;

    CalculateMaxCruiseSpeed(distance, startSpeed, cruiseSpeed, endSpeed, acceleration, accDist, decDist, maxCruiseSpeed, maxEndSpeed);

    const float accTime = fabsf(maxCruiseSpeed - startSpeed) / acceleration;
    const float decTime = fabsf(endSpeed - maxCruiseSpeed) / acceleration;
    const float cruiseDist = distance - accDist - decDist;
    const float cruiseTime = (maxCruiseSpeed != 0.0f) ? cruiseDist / maxCruiseSpeed : 0.0f;

    return accTime + cruiseTime + decTime;
}

