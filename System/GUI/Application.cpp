// This file is part of PadOS.
//
// Copyright (C) 2017-2021 Kurt Skauen <http://kavionic.com/>
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
// Created: 06.11.2017 23:22:03

#include <System/Platform.h>

#include <string.h>

#include <App/Application.h>
#include <GUI/View.h>
#include <GUI/Bitmap.h>
#include <ApplicationServer/ApplicationServer.h>
#include <ApplicationServer/DisplayDriver.h>

using namespace os;

Application* Application::s_DefaultApplication = nullptr;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Application* Application::GetDefaultApplication()
{
    return s_DefaultApplication;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Application::SetDefaultApplication(Application* application)
{
    s_DefaultApplication = application;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Application::Application(const String& name) : Looper(name, 1000), m_ReplyPort("app_reply", 1000)
{
    ASRegisterApplication::Sender::Emit(get_appserver_port(), -1, TimeValMicros::infinit, m_ReplyPort.GetHandle(), GetPortID(), GetName());

    m_LongPressTimer.Set(LONG_PRESS_DELAY, true);
    m_LongPressTimer.SignalTrigged.Connect(this, &Application::SlotLongPressTimer);

    for(;;)
    {
        MsgRegisterApplicationReply reply;
        int32_t                     code;
        if (m_ReplyPort.ReceiveMessage(nullptr, &code, &reply, sizeof(reply)))
        {
            if (code == AppserverProtocol::REGISTER_APPLICATION_REPLY) {
                m_ServerHandle = reply.m_ServerHandle;
                break;
            } else {
                printf("ERROR: Application::Application() received invalid reply: %" PRId32 "\n", code);
            }
        }
        else if (get_last_error() != EINTR)
        {
            printf("ERROR: Application::Application() receive failed: %s\n", strerror(get_last_error()));            
            break;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Application::~Application()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool Application::HandleMessage(handler_id targetHandler, int32_t code, const void* data, size_t length)
{
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Application::Idle()
{
    LayoutViews();
    Flush();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

IRect Application::GetScreenIFrame()
{
    return ApplicationServer::GetScreenIFrame();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool Application::AddView(Ptr<View> view, ViewDockType dockType, size_t index)
{
    if (dockType == ViewDockType::ChildView) {
        printf("Application::AddView() attempt to add top-level view as 'ViewDockType::ChildView'\n");
        return false;
    }
    view->HandlePreAttachToScreen(this);

    Ptr<View>   parent       = view->GetParent();
    handler_id  parentHandle = view->GetParentServerHandle();

    CreateServerView(view, parentHandle, dockType, index);

    for (Ptr<View> child : view->m_ChildrenList) {
        AddChildView(view, ptr_static_cast<View>(child));
    }
    view->HandleAttachedToScreen(this);
    LayoutViews();
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool Application::AddChildView(Ptr<View> parent, Ptr<View> view, size_t index)
{
    handler_id parentHandle = view->GetParentServerHandle();

    if (view->HasFlags(ViewFlags::WillDraw)) {
        CreateServerView(view, parentHandle, ViewDockType::ChildView, index);
    }
    for (Ptr<View> child : view->m_ChildrenList) {
        AddChildView(view, ptr_static_cast<View>(child));
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool Application::RemoveView(Ptr<View> view)
{
    if (view->m_ServerHandle != INVALID_HANDLE)
    {
        handle_id serverHandle = view->m_ServerHandle;
        DetachView(view);
        Post<ASDeleteView>(serverHandle);
    }
    view->HandleDetachedFromScreen();
    if (view->GetLooper() != nullptr) {
        RemoveHandler(view);
    }
    for (Ptr<View> child : *view) {
        RemoveView(child);
    }
    view->ClearFlags(ViewFlags::IsAttachedToScreen);
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<View> Application::FindView(handler_id handle)
{
    return ptr_static_cast<View>(FindHandler(handle));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Application::SetFocusView(MouseButton_e button, Ptr<View> view, bool focus)
{
    int deviceID = (button < MouseButton_e::FirstTouchID) ? 0 : int(button);

    if (view != nullptr)
    {
        Ptr<View> root = view;
        while (root->GetParent() != nullptr) root = root->GetParent();

        if (focus)
        {
            m_MouseFocusMap[deviceID] = ptr_raw_pointer_cast(view);
            Post<ASFocusView>(root->m_ServerHandle, button, true);
        }
        else
        {
            auto iterator = m_MouseFocusMap.find(deviceID);
            if (iterator != m_MouseFocusMap.end() && iterator->second == ptr_raw_pointer_cast(view))
            {
                m_MouseFocusMap.erase(iterator);
                Post<ASFocusView>(root->m_ServerHandle, button, false);
            }
        }
    }
    else
    {
        auto iterator = m_MouseFocusMap.find(deviceID);
        if (iterator != m_MouseFocusMap.end())
        {
            Ptr<View> root = ptr_tmp_cast(iterator->second);
            while (root->GetParent() != nullptr) root = root->GetParent();
            m_MouseFocusMap.erase(iterator);
            Post<ASFocusView>(root->m_ServerHandle, button, false);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<View> Application::GetFocusView(MouseButton_e button) const
{
    int deviceID = (button < MouseButton_e::FirstTouchID) ? 0 : int(button);

    auto iterator = m_MouseFocusMap.find(deviceID);
    if (iterator != m_MouseFocusMap.end()) {
        return ptr_tmp_cast(iterator->second);
    }
    return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Application::SetKeyboardFocus(Ptr<View> view, bool focus, bool notifyServer)
{
    if (!focus && view != m_KeyboardFocusView) {
        return;
    }

    View* newFocus = (focus) ? ptr_raw_pointer_cast(view) : nullptr;
    View* oldFocus = m_KeyboardFocusView;
    if (newFocus != oldFocus)
    {
        m_KeyboardFocusView = newFocus;

        if (oldFocus != nullptr) {
            oldFocus->OnKeyboardFocusChanged(false);
        }
        if (newFocus != nullptr) {
            newFocus->OnKeyboardFocusChanged(true);
        }

        if (notifyServer) {
            Post<ASSetKeyboardFocus>(view->m_ServerHandle, focus);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<View> Application::GetKeyboardFocus() const
{
    return ptr_tmp_cast(m_KeyboardFocusView);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool Application::CreateBitmap(int width, int height, ColorSpace colorSpace, uint32_t flags, handle_id& outHandle, uint8_t*& inOutFramebuffer, size_t& inOutBytesPerRow)
{
    Post<ASCreateBitmap>(m_ReplyPort.GetHandle(), width, height, colorSpace, (flags & Bitmap::CUSTOM_FRAMEBUFFER) ? inOutFramebuffer : nullptr, (flags & Bitmap::CUSTOM_FRAMEBUFFER) ? inOutBytesPerRow : 0, flags);
    Flush();

    for (;;)
    {
        MsgCreateBitmapReply reply;
        int32_t              code;
        if (m_ReplyPort.ReceiveMessage(nullptr, &code, &reply, sizeof(reply)))
        {
            if (code == AppserverProtocol::CREATE_BITMAP_REPLY)
            {
                if (reply.m_BitmapHandle == INVALID_HANDLE) {
                    return false;
                }
                outHandle         = reply.m_BitmapHandle;
                inOutFramebuffer  = reply.m_Framebuffer;
                inOutBytesPerRow  = reply.m_BytesPerRow;
                return true;
            }
            else
            {
                printf("ERROR: Application::CreateBitmap() received invalid reply: %" PRId32 "\n", code);
            }
        }
        else if (get_last_error() != EINTR)
        {
            printf("ERROR: Application::CreateBitmap() receive failed: %s\n", strerror(get_last_error()));
            return false;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Application::DeleteBitmap(handle_id bitmapHandle)
{
    Post<ASDeleteBitmap>(bitmapHandle);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Application::Flush()
{
    if (m_UsedSendBufferSize > 0) {
        send_message(get_appserver_port(), m_ServerHandle, AppserverProtocol::MESSAGE_BUNDLE, m_SendBuffer, m_UsedSendBufferSize);
        m_UsedSendBufferSize = 0;
    }    
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Application::Sync()
{
    Post<ASSync>(m_ReplyPort.GetHandle());
    Flush();
    int32_t code;
    for (;;)
    {
        if (m_ReplyPort.ReceiveMessage(nullptr, &code, nullptr, 0) < 0 && get_last_error() != EINTR) break;
        if (code == AppserverProtocol::SYNC_REPLY) {
            break;
        } else {
            printf("ERROR: Application::Sync() received invalid reply: %" PRIi32 "\n", code);
        }
        
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Application::DetachView(Ptr<View> view)
{
    for (auto i = m_MouseFocusMap.begin(); i != m_MouseFocusMap.end(); )
    {
        if (i->second == ptr_raw_pointer_cast(view)) {
            i = m_MouseFocusMap.erase(i);
        } else {
            ++i;
        }
    }
    for (auto i = m_MouseViewMap.begin(); i != m_MouseViewMap.end(); )
    {
        if (i->second == ptr_raw_pointer_cast(view)) {
            i = m_MouseViewMap.erase(i);
        } else {
            ++i;
        }
    }
    if (view == m_KeyboardFocusView) {
        m_KeyboardFocusView = nullptr;
        view->OnKeyboardFocusChanged(false);
    }
    view->SetServerHandle(INVALID_HANDLE);
    for (Ptr<View> child : *view)
    {
        DetachView(child);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void* Application::AllocMessageBuffer(int32_t messageID, size_t size)
{
    size += sizeof(AppserverMessage);
    if (size > sizeof(m_SendBuffer)) return nullptr;
    
    if (m_UsedSendBufferSize + size > sizeof(m_SendBuffer)) {
        Flush();
    }
    AppserverMessage* buffer = reinterpret_cast<AppserverMessage*>(m_SendBuffer + m_UsedSendBufferSize);
    m_UsedSendBufferSize += size;
    buffer->m_Code = messageID;
    buffer->m_Length = size;
    return buffer + 1;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool Application::CreateServerView(Ptr<View> view, handler_id parentHandle, ViewDockType dockType, size_t index)
{
    Post<ASCreateView>(GetPortID()
        , m_ReplyPort.GetHandle()
        , view->GetHandle()
        , parentHandle
        , dockType
        , index
        , view->GetName()
        , view->m_Frame + view->m_PositionOffset
        , view->m_ScrollOffset
        , view->m_Flags
        , view->m_HideCount
        , view->m_FocusKeyboardMode
        , view->m_DrawingMode
        , (view->m_Font != nullptr) ? view->m_Font->Get() : Font_e::e_FontLarge
        , view->m_EraseColor
        , view->m_BgColor
        , view->m_FgColor
        );
    Flush();

    for (;;)
    {
        MsgCreateViewReply reply;
        int32_t            code;
        if (m_ReplyPort.ReceiveMessage(nullptr, &code, &reply, sizeof(reply)))
        {
            if (code == AppserverProtocol::CREATE_VIEW_REPLY) {
                view->SetServerHandle(reply.m_ViewHandle);
                break;
            } else {
                printf("ERROR: Application::AddView() received invalid reply: %" PRId32 "\n", code);
            }
        }
        else if (get_last_error() != EINTR)
        {
            printf("ERROR: Application::AddView() receive failed: %s\n", strerror(get_last_error()));
            return false;
        }
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Application::RegisterViewForLayout(Ptr<View> view, bool recursive)
{
    if (!view->m_IsLayoutValid && !view->m_IsLayoutPending) {
        view->m_IsLayoutPending = true;
        m_ViewsNeedingLayout.insert(view);
    }
    if (recursive)
    {
        for (Ptr<View> child : view->GetChildList())
        {
            RegisterViewForLayout(child, true);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Application::SetMouseDownView(MouseButton_e button, Ptr<View> view, const MotionEvent& motionEvent)
{
    int deviceID = (button < MouseButton_e::FirstTouchID) ? 0 : int(button);

    if (view != nullptr)
    {
        m_MouseViewMap[deviceID] = ptr_raw_pointer_cast(view);
        m_LastClickEvent = motionEvent;
        m_LongPressTimer.Start(true);
    }
    else
    {
        auto iterator = m_MouseViewMap.find(deviceID);
        if (iterator != m_MouseViewMap.end()) {
            m_MouseViewMap.erase(iterator);
            if (button == m_LastClickEvent.ButtonID) {
                m_LongPressTimer.Stop();
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<View> Application::GetMouseDownView(MouseButton_e button) const
{
    int deviceID = (button < MouseButton_e::FirstTouchID) ? 0 : int(button);

    auto iterator = m_MouseViewMap.find(deviceID);
    if (iterator != m_MouseViewMap.end()) {
        return ptr_tmp_cast(iterator->second);
    }
    return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Application::LayoutViews()
{
    for (int i = 0; i < 100 && !m_ViewsNeedingLayout.empty(); ++i)
    {
        std::set<Ptr<View>> list = std::move(m_ViewsNeedingLayout);
        for (Ptr<View> view : list)
        {
            view->m_IsLayoutPending = false;
            view->UpdateLayout();
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Application::SlotLongPressTimer()
{
    Ptr<View> lastPressedView = GetMouseDownView(m_LastClickEvent.ButtonID);
    if (lastPressedView != nullptr) {
        lastPressedView->OnLongPress(m_LastClickEvent.ButtonID, lastPressedView->ConvertFromScreen(m_LastClickEvent.Position), m_LastClickEvent);
    }
}
