// This file is part of PadOS.
//
// Copyright (C) 2025 Kurt Skauen <http://kavionic.com/>
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
