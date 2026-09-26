// This file is part of PadOS.
//
// Copyright (c) 2020 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
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
