#pragma once

#include <GUI/View.h>
#include <DataTranslation/DataTranslator.h>


class PTextView;
class PBitmapView;
class PBitmap;
class PStreamableIO;
class PPath;


class PIconView : public PView
{
public:
    PIconView(const PString& name = PString::zero, Ptr<PView> parent = nullptr, uint32_t flags = 0);
    PIconView(PViewFactoryContext& context, Ptr<PView> parent, const pugi::xml_node& xmlData);

    Ptr<PBitmapView>         GetIconView();
    Ptr<const PBitmapView>   GetIconView() const;

    Ptr<PTextView>       GetLabelView();
    Ptr<const PTextView> GetLabelView() const;

    bool LoadBitmap(const PPath& path);
    bool LoadBitmap(PStreamableIO& file);

    void        SetBitmap(Ptr<PBitmap> bitmap);
    Ptr<PBitmap> GetBitmap() const;

    void SetLabel(const PString& label);

    void Clear();

    void SetHighlighting(bool isHighlighted);
    bool IsHighlighted() const { return m_IsHighlighted; }

    void SetSelection(bool isSelected);
    bool IsSelected() const { return m_IsSelected; }

private:
    void Construct();
    void UpdateHighlightColor();

    Ptr<PBitmapView> m_IconView;
    Ptr<PTextView>   m_LabelView;

    bool m_IsHighlighted = false;
    bool m_IsSelected = false;
};
