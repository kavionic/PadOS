// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <Ptr/Ptr.h>

struct PGUIStyleData
{
    // Owned by the widget; interpreted only by its current style.
    mutable Ptr<PtrTarget> StyleData;

    // Borrows the payload for this call without changing its reference count.
    template<class T>
    T* GetStyleData() const { return ptr_raw_pointer_static_cast<T>(StyleData); }

    template<class T>
    T& GetOrCreateStyleData() const
    {
        if (StyleData == nullptr) {
            StyleData = ptr_new<T>();
        }
        return *GetStyleData<T>();
    }
};
