// This file is part of PadOS.
//
// Copyright (c) 1999-2020 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <GUI/View.h>


///////////////////////////////////////////////////////////////////////////////
/// Base class for GUI controls.
/// \ingroup gui
/// \par Description:
///
/// \sa os::View
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////


class PControl : public PView
{
public:
    static constexpr int32_t INVALID_ID = -1;

    PControl(const PString& name = PString::zero, Ptr<PView> parent = nullptr, uint32_t flags = PViewFlags::WillDraw | PViewFlags::ClearBackground);
    PControl(PViewFactoryContext& context, Ptr<PView> parent, const pugi::xml_node& xmlData, PAlignment defaultLabelAlignment = PAlignment::Center);
    ~PControl();

      // From Control:
    virtual void OnEnableStatusChanged(bool isEnabled) { Invalidate(); Flush(); }
    virtual void OnLabelChanged(const PString& label) { Invalidate(); Flush(); PreferredSizeChanged(); }

    virtual void SetEnable(bool enabled);
    virtual bool IsEnabled() const;

    void    SetLabel(const PString& label);
    PString GetLabel() const { return m_Label; }
    
    void        SetLabelAlignment(PAlignment alignment) { m_LabelAlignment = alignment; PreferredSizeChanged(); Invalidate(); Flush(); }
    PAlignment   GetLabelAlignment() const { return m_LabelAlignment; }

    void    SetID(int32_t ID) { m_ID = ID; }
    int32_t GetID() const { return m_ID; }

private:
    int32_t m_ID = INVALID_ID;
    PString     m_Label;
    PAlignment   m_LabelAlignment = PAlignment::Center;
    bool    m_IsEnabled;
};
