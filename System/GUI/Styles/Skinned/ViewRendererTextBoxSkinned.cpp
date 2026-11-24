// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <algorithm>

#include <GUI/Style/ViewRendererContext.h>
#include <GUI/Style/ViewStyleTextBox.h>
#include <GUI/Styles/Skinned/SkinnedImage.h>
#include <GUI/Styles/Skinned/ViewRendererTextBoxSkinned.h>
#include <System/ExceptionHandling.h>

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PViewRendererTextBoxSkinned::PViewRendererTextBoxSkinned(const PTextBoxSkin& skin) : m_TextBoxSkin(skin)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<PtrTarget> PViewRendererTextBoxSkinned::Setup(PView& target) const
{
    return ptr_new<PViewRendererContext<PViewStyleTextBox>>(target);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewRendererTextBoxSkinned::Paint(PView& target, const PRect& frame, const PRect& updateRect, PtrTarget* context) const
{
    const PTextBoxStyleData& data = static_cast<PViewRendererContext<PViewStyleTextBox>*>(context)->GetTarget().GetTextBoxStyleData();
    const PTextBoxSkinState state = !data.IsEnabled ? PTextBoxSkinState::Disabled : data.IsReadOnly ? PTextBoxSkinState::ReadOnly : PTextBoxSkinState::Normal;
    const size_t stateIndex = std::to_underlying(state);
    const Ptr<const PBrush>& background = data.HasRaisedFrame ? m_TextBoxSkin.RaisedStates[stateIndex] : m_TextBoxSkin.States[stateIndex];
    const PDrawingMode previousMode = target.GetDrawingMode();
    PScopeExit restoreMode([&target, previousMode] { target.SetDrawingMode(previousMode); });
    target.SetBackgroundBrush(background);
    target.SetBackgroundBrushOrigin(frame.TopLeft());
    target.SetBrush(nullptr);
    target.SetDrawingMode(PDrawingMode::Copy);
    target.EraseRect(frame);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewRendererTextBoxSkinned::GetPreferredSize(const PView& target, PPoint* minSize, PPoint* maxSize, PtrTarget* context) const
{
    const PTextBoxStyleData& data = static_cast<PViewRendererContext<PViewStyleTextBox>*>(context)->GetTarget().GetTextBoxStyleData();
    *minSize = GetSizeForContent(target, data.EditorSize, context);
    *maxSize = *minSize;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewRendererTextBoxSkinned::PropertyChanged(PView& target, PTextBoxProperties properties, PtrTarget* context) const
{
    if (properties.Has(PTextBoxProperty::EditorSize)) {
        target.PreferredSizeChanged();
    }
    if (properties != PTextBoxProperty::EditorSize) {
        target.Invalidate();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PRect PViewRendererTextBoxSkinned::GetEditorFrame(const PView& target, PtrTarget* context) const
{
    return PInsetSkinnedFrame(target.GetBounds(), m_TextBoxSkin.EditorInsets);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PPoint PViewRendererTextBoxSkinned::GetSizeForContent(const PView& target, const PPoint& contentSize, PtrTarget* context) const
{
    const PRect& insets = m_TextBoxSkin.EditorInsets;
    return PPoint(
        std::max(m_TextBoxSkin.MinimumSize.x, contentSize.x + insets.left + insets.right),
        std::max(m_TextBoxSkin.MinimumSize.y, contentSize.y + insets.top + insets.bottom));
}
