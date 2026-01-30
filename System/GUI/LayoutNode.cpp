// This file is part of PadOS.
//
// Copyright (C) 2018-2025 Kurt Skauen <http://kavionic.com/>
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

#include "System/Platform.h"

#include <algorithm>
#include <stdio.h>
#include <limits.h>
#include <string.h>

#include "GUI/LayoutNode.h"
#include "GUI/View.h"


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PLayoutNode::PLayoutNode()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PLayoutNode::~PLayoutNode()
{
    if (m_View != nullptr) {
        m_View->SetLayoutNode(nullptr);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PLayoutNode::Layout()
{
    if (m_View != nullptr)
    {
        PRect frame   = m_View->GetNormalizedBounds();
    
        for (Ptr<PView> child : *m_View)
        {
            PRect borders = child->GetBorders();
            PRect childFrame = frame;
            childFrame.Resize(borders.left, borders.top, -borders.right, -borders.bottom);
            if (child->GetHAlignment() == PAlignment::Stretch && child->GetVAlignment() == PAlignment::Stretch)
            {
                child->SetFrame(childFrame);
            }
            else
            {
                const PPoint maxPrefferredSize = child->GetPreferredSize(PPrefSizeType::Greatest);
                PPoint offset(0.0f, 0.0f);

                const PPoint preferredSize = std::min(childFrame.Size(), maxPrefferredSize);

                switch(child->GetHAlignment())
                {
                    case PAlignment::Left:
                        offset.x = 0.0f;
                        childFrame.right = childFrame.left + preferredSize.x;
                        break;
                    case PAlignment::Center:
                        offset.x = roundf((childFrame.Width() - preferredSize.x) * 0.5f);
                        childFrame.right = childFrame.left + preferredSize.x;
                        break;
                    case PAlignment::Right:
                        offset.x = childFrame.Width() - preferredSize.x;
                        childFrame.right = childFrame.left + preferredSize.x;
                        break;
                    default:
                        break;
                }
                switch (child->GetVAlignment())
                {
                    case PAlignment::Top:
                        offset.y = 0.0f;
                        childFrame.bottom = childFrame.top + preferredSize.y;
                        break;
                    case PAlignment::Center:
                        offset.y = roundf((childFrame.Height() - preferredSize.y) * 0.5f);
                        childFrame.bottom = childFrame.top + preferredSize.y;
                        break;
                    case PAlignment::Right:
                        offset.y = childFrame.Height() - preferredSize.y;
                        childFrame.bottom = childFrame.top + preferredSize.y;
                        break;
                    default:
                        break;
                }
                child->SetFrame(childFrame + offset);
            }
        }            
    }
    
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PLayoutNode::CalculatePreferredSize(PPoint* minSizeOut, PPoint* maxSizeOut, bool includeWidth, bool includeHeight)
{
    PPoint minSize(0.0f,0.0f);
    PPoint maxSize(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
    
    if (m_View != nullptr)
    {
        for (Ptr<PView> child : *m_View)
        {
            if (child->HasFlags(PViewFlags::IgnoreWhenHidden) && !child->IsVisible()) {
                continue;
            }
            const PRect borders = child->GetBorders();
            const PPoint borderSize(borders.left + borders.right, borders.top + borders.bottom);
            const PPoint currentMinSize = child->GetPreferredSize(PPrefSizeType::Smallest) + borderSize;
            const PPoint currentMaxSize = child->GetPreferredSize(PPrefSizeType::Greatest) + borderSize;

            if (includeWidth) {
                minSize.x = std::max(minSize.x, currentMinSize.x);
                maxSize.x = std::min(maxSize.x, currentMaxSize.x);
            }
            if (includeHeight) {
                minSize.y = std::max(minSize.y, currentMinSize.y);
                maxSize.y = std::min(maxSize.y, currentMaxSize.y);
            }                
        }
        if (includeWidth) {
            minSizeOut->x = std::ceil(minSize.x);
            maxSizeOut->x = std::floor(maxSize.x);
        }            
        if (includeHeight) {
            minSizeOut->y = std::ceil(minSize.y);
            maxSizeOut->y = std::floor(maxSize.y);
        }            
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PLayoutNode::ApplyInnerBorders(const PRect& borders, float spacing)
{
    if (m_View != nullptr)
    {
        for (Ptr<PView> child : *m_View)
        {
            PRect childBorders = child->GetBorders();
            childBorders.Resize(borders.left, borders.top, borders.right, borders.bottom);
            child->SetBorders(childBorders);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PLayoutNode::AttachedToView(PView* view)
{
    if (m_View != nullptr) {
        m_View->SetLayoutNode(nullptr);
    }
    m_View = view;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static float SpaceOut(uint32_t count, float totalSize, float totalMinSize, float totalWheight,
                      float* minSizes, float* maxSizes, float* wheights, float* finalSizes)
{
    float extraSpace = totalSize - totalMinSize;
    
    bool*  doneFlags = (bool*)alloca(sizeof(bool) * count);

    memset(doneFlags, 0, sizeof(bool) * count);

    for (;;)
    {
        uint32_t i;
        for (i = 0 ; i < count ; ++i)
        {
            if (doneFlags[i]) {
                continue;
            }
            if (wheights[i] == 0.0f)
            {
                finalSizes[i] = 0.0f;
                doneFlags[i] = true;
                continue;
            }
            float vWeight = wheights[i] / totalWheight;

            finalSizes[i] = minSizes[i] + extraSpace * vWeight;
            if (finalSizes[i] >= maxSizes[i])
            {
                extraSpace   -= maxSizes[i] - minSizes[i];
                totalWheight -= wheights[i];
                finalSizes[i] = maxSizes[i];
                doneFlags[i] = true;
                break;
            }
        }
        if (i == count) {
            break;
        }
    }
    for (uint32_t i = 0 ; i < count ; ++i) {
        totalSize -= finalSizes[i];
    }
    return totalSize;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PHLayoutNode::PHLayoutNode()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PHLayoutNode::CalculatePreferredSize(PPoint* minSize, PPoint* maxSize, bool includeWidth, bool includeHeight)
{
    *minSize = PPoint(0.0f, 0.0f);
    *maxSize = PPoint(0.0f, 0.0f);

    if (m_View != nullptr)
    {
        for (Ptr<PView> child : *m_View)
        {
            if (child->HasFlags(PViewFlags::IgnoreWhenHidden) && !child->IsVisible()) {
                continue;
            }
            PRect borders = child->GetBorders();
            PPoint borderSize(borders.left + borders.right, borders.top + borders.bottom);
            PPoint childMinSize = child->GetPreferredSize(PPrefSizeType::Smallest) + borderSize;
            PPoint childMaxSize = child->GetPreferredSize(PPrefSizeType::Greatest) + borderSize;
            if (includeWidth) {
                minSize->x += childMinSize.x;
                maxSize->x += childMaxSize.x;
            }
            if (includeHeight) {
                if (childMinSize.y > minSize->y) minSize->y = childMinSize.y;
                if (childMaxSize.y > maxSize->y) maxSize->y = childMaxSize.y;
            }                
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PHLayoutNode::ApplyInnerBorders(const PRect& borders, float spacing)
{
    if (m_View == nullptr) {
        return;
    }
    const auto& childList = m_View->GetChildList();
    if (childList.empty()) {
        return;
    }
    if (childList.size() == 1)
    {
        PRect childBorders = childList[0]->GetBorders();
        childBorders.Resize(borders.left, borders.top, borders.right, borders.bottom);
        childList[0]->SetBorders(childBorders);
    }
    else
    {
        // Apply to left-most child:
        PRect childBorders = childList[0]->GetBorders();
        childBorders.Resize(borders.left, borders.top, spacing, borders.bottom);
        childList[0]->SetBorders(childBorders);
		
		// Apply to right-most child:
        childBorders = childList[childList.size() - 1]->GetBorders();
		childBorders.Resize(0.0f, borders.top, borders.right, borders.bottom);
		childList[childList.size() - 1]->SetBorders(childBorders);
		
        // Apply to center children:
        for (size_t i = 1; i < childList.size() - 1; ++i)
        {
			childBorders = childList[i]->GetBorders();
			childBorders.Resize(0.0f, borders.top, spacing, borders.bottom);
			childList[i]->SetBorders(childBorders);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PHLayoutNode::Layout()
{
    if (m_View != nullptr)
    {
        const auto& childList = m_View->GetChildList();

        PRect bounds = m_View->GetNormalizedBounds();

        float* minWidths    = (float*)alloca(sizeof(float)*childList.size());
        float* maxWidths    = (float*)alloca(sizeof(float)*childList.size());
        float* maxHeights   = (float*)alloca(sizeof(float)*childList.size());
        float* wheights     = (float*)alloca(sizeof(float)*childList.size());
        float* finalHeights = (float*)alloca(sizeof(float)*childList.size());
            
        float  vMinWidth = 0.0f;
        float  totalWheight = 0.0f;
    
        for (size_t i = 0 ; i < childList.size() ; ++i)
        {
            Ptr<PView> child = childList[i];
  
            if (child->HasFlags(PViewFlags::IgnoreWhenHidden) && !child->IsVisible())
            {
                wheights[i] = 0.0f;
                maxHeights[i] = 0.0f;
                minWidths[i] = 0.0f;
                maxWidths[i] = 0.0f;
                continue;
            }

            const PRect borders = child->GetBorders();
            const PPoint borderSize(borders.left + borders.right, borders.top + borders.bottom);
            
            const PPoint minSize = child->GetPreferredSize(PPrefSizeType::Smallest) + borderSize;
            const PPoint maxSize = child->GetPreferredSize(PPrefSizeType::Greatest) + borderSize;

            
            wheights[i]   = child->GetWheight();

            maxHeights[i] = maxSize.y;
            maxWidths[i]  = maxSize.x;
            minWidths[i]  = minSize.x;

            vMinWidth += minSize.x;
            totalWheight += wheights[i];
        }
    //    if ( vMinWidth > bounds.Width() + 1.0f ) {
    //      p_system_log<PLogSeverity::ERROR>(LogCategoryGUITK, "HLayoutNode::Layout() Width={:.2f}, Required width={:.2f}", bounds.Width() + 1.0f, vMinWidth  );
    //    }
        const float unusedWidth = SpaceOut(childList.size(), bounds.Width(), vMinWidth, totalWheight, minWidths, maxWidths, wheights, finalHeights) / float(childList.size());

        float x = bounds.left + unusedWidth * 0.5f;
    
        for (size_t i = 0 ; i < childList.size() ; ++i)
        {
            PRect frame(0.0f, 0.0f, finalHeights[i], bounds.bottom);
            if (childList[i]->GetVAlignment() != PAlignment::Stretch && frame.Height() > maxHeights[i]) {
                frame.bottom = maxHeights[i];
            }
            float y = 0.0f;
            switch(childList[i]->GetVAlignment())
            {
                case PAlignment::Top:    y = bounds.top; break;
                case PAlignment::Right:  y = bounds.bottom - frame.Height(); break;
                default:           p_system_log<PLogSeverity::ERROR>(LogCategoryGUITK, "HLayoutNode::Layout() node '{}' has invalid v-alignment {}", m_View->GetName(), int(childList[i]->GetVAlignment()) );
                [[fallthrough]];
                case PAlignment::Center: y = bounds.top + (bounds.Height() - frame.Height()) * 0.5f; break;
                case PAlignment::Stretch: x = 0.0f; break;
            }
            
            frame += PPoint(x, y);
            PRect borders = childList[i]->GetBorders();
            
            float width = frame.Width();
            frame.Resize(borders.left, borders.top, -borders.right, -borders.bottom);
            
            x += width + unusedWidth;
            frame.Floor();
//            p_system_log<PLogSeverity::ERROR>(LogCategoryGUITK, "    {}: {:.2f}->{:.2f}", i, frame.left, frame.right);
            childList[i]->SetFrame(frame);
        }
    }    
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PVLayoutNode::PVLayoutNode()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PVLayoutNode::CalculatePreferredSize(PPoint* minSize, PPoint* maxSize, bool includeWidth, bool includeHeight)
{
    *minSize = PPoint(0.0f, 0.0f);
    *maxSize = PPoint(0.0f, 0.0f);
    
    if (m_View != nullptr)
    {
        for (Ptr<PView> child : *m_View)
        {
            if (child->HasFlags(PViewFlags::IgnoreWhenHidden) && !child->IsVisible()) {
                continue;
            }

            const PRect  borders = child->GetBorders();
            const PPoint borderSize(borders.left + borders.right, borders.top + borders.bottom);

            const PPoint childMinSize = child->GetPreferredSize(PPrefSizeType::Smallest) + borderSize;
            const PPoint childMaxSize = child->GetPreferredSize(PPrefSizeType::Greatest) + borderSize;
            
            if (includeWidth) {
                if (childMinSize.x > minSize->x) minSize->x = childMinSize.x;
                if (childMaxSize.x > maxSize->x) maxSize->x = childMaxSize.x;
            }
            if (includeHeight) {
                minSize->y += childMinSize.y;
                maxSize->y += childMaxSize.y;
            }
        }
    }        
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PVLayoutNode::Layout()
{
    if (m_View != nullptr)
    {
        const auto& childList = m_View->GetChildList();
        const PRect bounds = m_View->GetNormalizedBounds();
    
        float* maxWidths    = (float*)alloca(sizeof(float)*childList.size());
        float* minHeights   = (float*)alloca(sizeof(float)*childList.size());
        float* maxHeights   = (float*)alloca(sizeof(float)*childList.size());
        float* wheights     = (float*)alloca(sizeof(float)*childList.size());
        float* finalHeights = (float*)alloca(sizeof(float)*childList.size());

        float  totalWheight = 0.0f;
        float  vMinHeight = 0.0f;
        float  vMaxHeight = 0.0f;

        for (size_t i = 0 ; i < childList.size() ; ++i)
        {
            Ptr<PView> child = childList[i];
            
            if (child->HasFlags(PViewFlags::IgnoreWhenHidden) && !child->IsVisible())
            {
                wheights[i] = 0.0f;
                maxWidths[i] = 0.0f;
                minHeights[i] = 0.0f;
                maxHeights[i] = 0.0f;
                continue;
            }

            const PRect borders = child->GetBorders();
            const PPoint borderSize(borders.left + borders.right, borders.top + borders.bottom);
            
            const PPoint minSize = child->GetPreferredSize(PPrefSizeType::Smallest) + borderSize;
            const PPoint maxSize = child->GetPreferredSize(PPrefSizeType::Greatest) + borderSize;


            wheights[i]   = child->GetWheight();
            maxWidths[i]  = maxSize.x;
            minHeights[i] = minSize.y;
            maxHeights[i] = maxSize.y;

            vMinHeight += minSize.y;
            vMaxHeight += maxSize.y;
            totalWheight += wheights[i];
        }
    //    if ( vMinHeight > bounds.Height() + 1.0f ) {
    //      printf( "Error: HLayoutNode::Layout() Width=%.2f, Required width=%.2f\n", bounds.Height() + 1.0f, vMinHeight  );
    //    }
        const float vUnusedHeight = SpaceOut(childList.size(), bounds.Height(),  vMinHeight, totalWheight, minHeights, maxHeights, wheights, finalHeights) / float(childList.size());

        float y = bounds.top + vUnusedHeight * 0.5f;
        for (size_t i = 0 ; i < childList.size() ; ++i)
        {
            PRect frame(0.0f, 0.0f, bounds.right, finalHeights[i]);
            if (childList[i]->GetHAlignment() != PAlignment::Stretch && frame.Width() > maxWidths[i]) {
                frame.right = maxWidths[i];
            }
            float x;
            switch(childList[i]->GetHAlignment())
            {
                case PAlignment::Left:   x = bounds.left; break;
                case PAlignment::Right:  x = bounds.right - frame.Width(); break;
                default:           p_system_log<PLogSeverity::ERROR>(LogCategoryGUITK, "VLayoutNode::Layout() node '{}' has invalid h-alignment {}", m_View->GetName(), int(childList[i]->GetHAlignment()) );
                [[fallthrough]];
                case PAlignment::Center: x = bounds.left + (bounds.Width() - frame.Width()) * 0.5f; break;
                case PAlignment::Stretch: x = 0.0f; break;
            }
            frame += PPoint(x, y);
            
            float height = frame.Height();
            PRect borders = childList[i]->GetBorders();
            frame.Resize(borders.left, borders.top, -borders.right, -borders.bottom);
            y += height + vUnusedHeight;
            frame.Floor();
            childList[i]->SetFrame(frame);
        }
    }        
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PVLayoutNode::ApplyInnerBorders(const PRect& borders, float spacing)
{
	if (m_View == nullptr) {
		return;
	}
	const auto& childList = m_View->GetChildList();
	if (childList.empty()) {
		return;
	}
	if (childList.size() == 1)
	{
		PRect childBorders = childList[0]->GetBorders();
		childBorders.Resize(borders.left, borders.top, borders.right, borders.bottom);
		childList[0]->SetBorders(childBorders);
	}
    else
	{
		// Apply to top child:
		PRect childBorders = childList[0]->GetBorders();
		childBorders.Resize(borders.left, borders.top, borders.right, spacing);
		childList[0]->SetBorders(childBorders);

		// Apply to bottom child:
		childBorders = childList[childList.size() - 1]->GetBorders();
		childBorders.Resize(borders.left, 0.0f, borders.right, borders.bottom);
		childList[childList.size() - 1]->SetBorders(childBorders);

		// Apply to center children:
		for (size_t i = 1; i < childList.size() - 1; ++i)
		{
			childBorders = childList[i]->GetBorders();
			childBorders.Resize(borders.left, 0.0f, borders.right, spacing);
			childList[i]->SetBorders(childBorders);
		}
	}
}
