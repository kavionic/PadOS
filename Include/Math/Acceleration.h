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

#pragma once


namespace PAcceleration
{

float CalculateAccelerationSpeed(float distance, float startSpeed, float acceleration);
float CalcAccelerationDistance(float startSpeed, float resultSpeed, float acceleration);
float CalcMaxSpeedPermittingStop(float distance, float startSpeed, float endSpeed, float acceleration);
bool  CalculateMaxCruiseSpeed(float distance, float startSpeed, float cruiseSpeed, float endSpeed, float acceleration, float& outAccDist, float& outDecDist, float& outMaxCruiseSpeed, float& outMaxEndSpeed);
float CalcTravelTime(float distance, float startSpeed, float cruiseSpeed, float endSpeed, float acceleration);

} // namespace Acceleration
