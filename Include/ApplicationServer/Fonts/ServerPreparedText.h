// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <ApplicationServer/Fonts/ServerFontDrawCache.h>
#include <GUI/FontDescription.h>
#include <Ptr/Ptr.h>

struct PServerPreparedText
{
    Ptr<PServerFontDrawCache> Cache;
    PFontTextMetrics Metrics;
    size_t KerningCount = 0;
};
