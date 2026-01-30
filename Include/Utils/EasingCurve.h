// This file is part of PadOS.
//
// Copyright (C) 2020 Kurt Skauen <http://kavionic.com/>
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
// Created: 08.09.2020 22:30

#pragma once

#include <Math/Misc.h>


enum class PEasingCurveFunction
{
    Linear,
    EaseIn,
    EaseOut,
    EaseInOut
};

class PEasingCurve
{
public:
    PEasingCurve(PEasingCurveFunction function) : m_Function(function) {}
    float   GetValue(float progress) const
    {
        switch (m_Function)
        {
            case PEasingCurveFunction::Linear:
                return progress;
            case PEasingCurveFunction::EaseIn:
                return PMath::square(progress);
            case PEasingCurveFunction::EaseOut:
                return 1.0f - PMath::square(1.0f - progress);
                break;
            case PEasingCurveFunction::EaseInOut:
                if (progress < 0.5f) {
                    return PMath::square(progress * 2.0f) * 0.5f;
                }
                else {
                    return 0.5f + PMath::square((0.5f - progress) * 2.0f) * 0.5f;
                }
                break;
            default:
                return progress;
        }
    }
private:
    PEasingCurveFunction m_Function;
};
