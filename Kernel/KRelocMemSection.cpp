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
// Created: 21.11.2025 23:30

#include <Kernel/KRelocMemSection.h>


void krelocate_memory_sections(const KRelocMemSection* sections, size_t count)
{
    for (size_t i = 0; i < count; ++i)
    {
        const KRelocMemSection& section = sections[i];

        if (section.Source != section.Destination)
        {
            kassert((section.Size & 3) == 0);

            const uint32_t* src = reinterpret_cast<const uint32_t*>(section.Source);
            uint32_t*       dst = reinterpret_cast<uint32_t*>(section.Destination);
            const size_t    words = section.Size / 4;

            for (size_t j = 0; j < words; ++j) {
                *dst++ = *src++;
            }
        }
    }
}
