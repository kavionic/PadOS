// This file is part of PadOS.
//
// Copyright (c) 2021 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
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
