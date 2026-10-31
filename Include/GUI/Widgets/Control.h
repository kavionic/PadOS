// This file is part of PadOS.
//
// Copyright (c) 1999-2020 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <GUI/StyledView.h>


///////////////////////////////////////////////////////////////////////////////
/// Base class for GUI controls.
/// \ingroup gui
/// \par Description:
///
/// \sa os::View
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////


class PControl : public PStyledView
{
public:
    static constexpr int32_t INVALID_ID = -1;

    PControl(const PString& name = PString::zero, uint32_t flags = PViewFlags::WillDraw | PViewFlags::ClearBackground);
    PControl(PViewFactoryContext& context, const pugi::xml_node& xmlData);
    ~PControl();

      // From Control:
    virtual void OnEnableStatusChanged(bool isEnabled) { Invalidate(); Flush(); }

    virtual void SetEnable(bool enabled);
    virtual bool IsEnabled() const;

    void    SetID(int32_t ID) { m_ID = ID; }
    int32_t GetID() const { return m_ID; }

private:
    int32_t m_ID = INVALID_ID;
    bool    m_IsEnabled;
};
