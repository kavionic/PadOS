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

#include <algorithm>
#include <Utils/PIDController.h>

using namespace os;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PIDController::Reset()
{
    m_PrevError = 0.0f;
    m_IntegralError = 0.0f;
    m_CurrentControlValue = m_ControlMin;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

float PIDController::Update(float deltaTime, float measuredValue)
{
    const float error = m_TargetValue - measuredValue;

    if (deltaTime <= 0.0f)
    {
        m_PrevError = error;
        m_CurrentControlValue = std::clamp(error * m_PGain, m_ControlMin, m_ControlMax);
        return m_CurrentControlValue;
    }

    const float derivative = (error - m_PrevError) / deltaTime;
    m_PrevError = error;

    if (m_PGain > 0.0f)
    {
        if ((m_IntegralError + error) * m_PGain > m_ControlMin && (m_IntegralError + error) * m_PGain < m_ControlMax) {
            m_IntegralError = std::clamp(m_IntegralError + error * deltaTime * m_IGain, m_ControlMin / m_PGain, m_ControlMax / m_PGain);
        }
    }
    else
    {
        m_IntegralError = 0.0f;
    }

    const float NewControlValue = (error + m_IntegralError) * m_PGain + derivative * m_DGain;
    const float smoothFraction = deltaTime * m_ControlSmoothingInverseTimeConstant;
    if (smoothFraction < 1.0f) {
        m_CurrentControlValue += (NewControlValue - m_CurrentControlValue) * smoothFraction;
    } else {
        m_CurrentControlValue = NewControlValue;
    }

    m_CurrentControlValue = std::clamp(m_CurrentControlValue, m_ControlMin, m_ControlMax);

    return m_CurrentControlValue;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PIDController::SetControlSmoothingTimeConstant(float timeConstant)
{
    if (timeConstant != 0.0f) {
        m_ControlSmoothingInverseTimeConstant = 1.0f / timeConstant;
    } else {
        m_ControlSmoothingInverseTimeConstant = std::numeric_limits<float>::max();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

float PIDController::GetControlSmoothingTimeConstant() const
{
    if (m_ControlSmoothingInverseTimeConstant != 0.0f) {
        return 1.0f / m_ControlSmoothingInverseTimeConstant;
    } else {
        return std::numeric_limits<float>::max();
    }
}
