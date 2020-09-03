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
// Created: 09.02.2020 20:10

#pragma once

template<typename ARRAY>
constexpr void combsort(ARRAY& dstArray) noexcept
{
    size_t              gap = dstArray.size();
    constexpr double    shrink = 1.0 / 1.247330950103979;
    bool                swapped = false;

    while (gap > 1 || swapped)
    {
        swapped = false;

        if (gap > 1) {
            gap = static_cast<size_t>(double(gap) * shrink);
        }
        for (size_t i = 0; gap + i < dstArray.size(); ++i)
        {
            if (dstArray[i + gap] < dstArray[i])
            {
                auto swap = dstArray[i];
                dstArray[i] = dstArray[i + gap];
                dstArray[i + gap] = swap;
                swapped = true;
            }
        }
    }
}

template<typename ARRAY>
constexpr ARRAY combsort_immutable(ARRAY&& srcArray) noexcept
{
    auto dstArray = std::move(srcArray);
    combsort(dstArray);
    return dstArray;
}
