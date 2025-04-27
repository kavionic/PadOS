// This file is part of PadOS.
//
// Copyright (C) 2024 Kurt Skauen <http://kavionic.com/>
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
// Created: 17.11.2024 22:30

#pragma once

#include <System/SysTime.h>

namespace os
{

class PIDController
{
public:
    PIDController();

    void    Reset();
    float   Update(float deltaTime, float measuredValue);

    void    SetTargetValue(float targetValue);

    void    SetPGain(float gain) { m_PGain = gain; }
    float   GetPGain() const { return m_PGain; }

    void    SetIGain(float gain) { m_IGain = gain; }
    float   GetIGain() const { return m_IGain; }

    void    SetDGain(float gain) { m_DGain = gain; }
    float   GetDGain() const { return m_DGain; }

    void    SetControlRange(float controlMin, float controlMax) { m_ControlMin = controlMin; m_ControlMax = controlMax; }
    float   GetControlMin() const { return m_ControlMin; }
    float   GetControlMax() const { return m_ControlMax; }

    void    SetControlSmoothingTimeConstant(float timeConstant);
    float   GetControlSmoothingTimeConstant() const;

private:
    float m_PGain = 0.0f;
    float m_IGain = 0.0f;
    float m_DGain = 0.0f;

    float m_ControlSmoothingInverseTimeConstant = 1.0f;

    float m_SetpointWindowSpacing = 1.0f / 2.0f;
    float m_ControlMin = 0.0f;
    float m_ControlMax = 1.0f;

    float m_TargetValue = 0.0f;

    float               m_PrevError = 0.0f;
    float               m_CurrentControlValue = 0.0f;
    size_t              m_CurrentSetpointWindow = 0;
    std::vector<float>  m_IntegralErrors;
};


} // namespace os