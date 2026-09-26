// This file is part of PadOS.
//
// Copyright (c) 2020 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
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
