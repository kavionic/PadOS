#pragma once

#include <GUI/View.h>
#include <DataTranslation/DataTranslator.h>

namespace os
{

class StreamableIO;
class Path;
class Bitmap;
class BitmapView;
class TextView;

class IconView : public View
{
public:
    IconView(const String& name = String::zero, Ptr<View> parent = nullptr, uint32_t flags = 0);
    IconView(ViewFactoryContext& context, Ptr<View> parent, const pugi::xml_node& xmlData);

    bool LoadBitmap(const Path& path);
    bool LoadBitmap(StreamableIO& file);

    void        SetBitmap(Ptr<Bitmap> bitmap);
    Ptr<Bitmap> GetBitmap() const;

    void SetLabel(const String& label);

    void Clear();

    void SetHighlighting(bool isHighlighted);
    bool IsHighlighted() const { return m_IsHighlighted; }

    void SetSelection(bool isSelected);
    bool IsSelected() const { return m_IsSelected; }

private:
    void Construct();
    void UpdateHighlightColor();

    Ptr<BitmapView> m_IconView;
    Ptr<TextView>   m_LabelView;

    bool m_IsHighlighted = false;
    bool m_IsSelected = false;
};

} // namespace os


