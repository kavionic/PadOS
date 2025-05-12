// This file is part of PadOS.
//
// Copyright (C) 1999-2020 Kurt Skauen <http://kavionic.com/>
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

#include <GUI/Bitmap.h>
#include <App/Application.h>

using namespace os;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Bitmap::Bitmap(int width, int height, EColorSpace colorSpace, uint32_t flags, Application* application)
{
    Initialize(width, height, colorSpace, nullptr, 0, flags, application);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Bitmap::Bitmap(int width, int height, EColorSpace colorSpace, void* raster, size_t bytesPerRow, uint32_t flags, Application* application)
{
    Initialize(width, height, colorSpace, static_cast<uint8_t*>(raster), bytesPerRow, flags | CUSTOM_FRAMEBUFFER, application);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Bitmap::Bitmap(int width, int height, EColorSpace colorSpace, const void* raster, size_t bytesPerRow, uint32_t flags, Application* application)
{
    Initialize(width, height, colorSpace, static_cast<uint8_t*>(const_cast<void*>(raster)), bytesPerRow, flags | CUSTOM_FRAMEBUFFER | READ_ONLY, application);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Bitmap::Initialize(int width, int height, EColorSpace colorSpace, uint8_t* raster, size_t bytesPerRow, uint32_t flags, Application* application)
{
    m_Bounds = IRect(0, 0, width, height);

    m_Handle        = INVALID_HANDLE;
    m_Raster        = raster;
    m_ColorSpace    = colorSpace;
    m_BytesPerRow   = bytesPerRow;

    if (application == nullptr) {
        application = Application::GetCurrentApplication();
    }
    if (application != nullptr)
    {
        if (application->CreateBitmap(width, height, colorSpace, flags, m_Handle, m_Raster, m_BytesPerRow)) {
            m_Application = application;
        }
    }
//    if (flags & ACCEPT_VIEWS) {
//        m_pcWindow = new Window(this);
//        m_pcWindow->Unlock();
//    } else {
//        m_pcWindow = NULL;
//    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Bitmap::~Bitmap()
{
    if (m_Application != nullptr) {
        m_Application->DeleteBitmap(m_Handle);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

EColorSpace Bitmap::GetColorSpace() const
{
    return m_ColorSpace;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Bitmap::AddChild(View* pcView)
{
//    if (NULL != m_pcWindow) {
//        m_pcWindow->AddChild(pcView);
//    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool Bitmap::RemoveChild(View* pcView)
{
//    if (NULL != m_pcWindow) {
//        m_pcWindow->RemoveChild(pcView);
//        return true;
//    } else {
        return false;
//    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

View* Bitmap::FindView(const char* pzName) const
{
//    if (NULL != m_pcWindow) {
//        return m_pcWindow->FindView(pzName);
//    } else {
        return nullptr;
//    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Bitmap::Flush()
{
//    if (NULL != m_pcWindow) {
//        m_pcWindow->Flush();
//    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Bitmap::Sync()
{
//    if (NULL != m_pcWindow) {
//        m_pcWindow->Sync();
//    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

IRect Bitmap::GetBounds() const
{
    return m_Bounds;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int Bitmap::GetBytesPerRow() const
{
    return m_BytesPerRow;
}
