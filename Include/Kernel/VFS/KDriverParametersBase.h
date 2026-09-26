// This file is part of PadOS.
//
// Copyright (c) 2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 05.11.2025 22:00

#pragma once

#include <Utils/String.h>
#include <Utils/JSON.h>

struct KDriverParametersBase
{
    KDriverParametersBase() = default;
    KDriverParametersBase(const PString& devicePath) : DevicePath(devicePath) {}

    PString      DevicePath;

    friend void to_json(Pjson& data, const KDriverParametersBase& value)
    {
        data = Pjson{ {"device_path", value.DevicePath } };
    }
    friend void from_json(const Pjson& data, KDriverParametersBase& outValue)
    {
        data.at("device_path").get_to(outValue.DevicePath);
    }
};
