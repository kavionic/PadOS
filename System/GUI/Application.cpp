// This file is part of PadOS.
//
// Copyright (C) 2017-2025 Kurt Skauen <http://kavionic.com/>
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


PApplication* PApplication::s_DefaultApplication = nullptr;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PApplication* PApplication::GetDefaultApplication()
{
    return s_DefaultApplication;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PApplication::SetDefaultApplication(PApplication* application)
{
    s_DefaultApplication = application;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PApplication::PApplication(const PString& name) : PLooper(name, 1000), m_ReplyPort("app_reply", 1000)
{
    p_post_to_remotesignal<ASRegisterApplication>(p_get_appserver_port(), INVALID_HANDLE, TimeValNanos::infinit, m_ReplyPort.GetHandle(), GetPortID(), GetName());

    m_LongPressTimer.Set(LONG_PRESS_DELAY, true);
    m_LongPressTimer.SignalTrigged.Connect(this, &PApplication::SlotLongPressTimer);

    for(;;)
    {
        MsgRegisterApplicationReply reply;
        int32_t                     code;
        if (m_ReplyPort.ReceiveMessage(nullptr, &code, &reply, sizeof(reply)))
        {
            if (code == PAppserverProtocol::REGISTER_APPLICATION_REPLY)
            {
                m_ServerHandle = reply.m_ServerHandle;
                break;
            }
            else
            {
                p_system_log<PLogSeverity::ERROR>(LogCategoryGUITK, "Application::Application() received invalid reply: {}", code);
            }
        }
        else if (get_last_error() != EINTR)
        {
            p_system_log<PLogSeverity::ERROR>(LogCategoryGUITK, "Application::Application() receive failed: {}", strerror(get_last_error()));
            break;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PApplication::~PApplication()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PApplication::HandleMessage(handler_id targetHandler, int32_t code, const void* data, size_t length)
{
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PApplication::Idle()
{
    LayoutViews();
    Flush();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PIRect PApplication::GetScreenIFrame()
{
    return ApplicationServer::GetScreenIFrame();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PApplication::AddView(Ptr<PView> view, PViewDockType dockType, size_t index)
{
    assert(!IsRunning() || GetMutex().IsLocked());

    if (dockType == PViewDockType::ChildView) {
        p_system_log<PLogSeverity::ERROR>(LogCategoryGUITK, "Application::AddView() attempt to add top-level view as 'ViewDockType::ChildView'");
        return false;
    }
    view->HandlePreAttachToScreen(this);

    Ptr<PView>   parent       = view->GetParent();
    handler_id  parentHandle = view->GetParentServerHandle();

    CreateServerView(view, parentHandle, dockType, index);

    for (Ptr<PView> child : view->m_ChildrenList) {
        AddChildView(view, ptr_static_cast<PView>(child));
    }
    view->HandleAttachedToScreen(this);
    LayoutViews();
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PApplication::AddChildView(Ptr<PView> parent, Ptr<PView> view, size_t index)
{
    assert(!IsRunning() || GetMutex().IsLocked());

    handler_id parentHandle = view->GetParentServerHandle();

    if (view->HasFlags(PViewFlags::WillDraw)) {
        CreateServerView(view, parentHandle, PViewDockType::ChildView, index);
    }
    for (Ptr<PView> child : view->m_ChildrenList) {
        AddChildView(view, ptr_static_cast<PView>(child));
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PApplication::RemoveView(Ptr<PView> view)
{
    assert(!IsRunning() || GetMutex().IsLocked());

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
    for (Ptr<PView> child : *view) {
        RemoveView(child);
    }
    view->ClearFlags(PViewFlags::IsAttachedToScreen);
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<PView> PApplication::FindView(handler_id handle)
{
    assert(!IsRunning() || GetMutex().IsLocked());

    return ptr_static_cast<PView>(FindHandler(handle));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PApplication::SetFocusView(PMouseButton button, Ptr<PView> view, bool focus)
{
    assert(!IsRunning() || GetMutex().IsLocked());

    int deviceID = (button < PMouseButton::FirstTouchID) ? 0 : int(button);

    if (view != nullptr)
    {
        Ptr<PView> root = view;
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
            Ptr<PView> root = ptr_tmp_cast(iterator->second);
            while (root->GetParent() != nullptr) root = root->GetParent();
            m_MouseFocusMap.erase(iterator);
            Post<ASFocusView>(root->m_ServerHandle, button, false);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<PView> PApplication::GetFocusView(PMouseButton button) const
{
    assert(!IsRunning() || GetMutex().IsLocked());

    int deviceID = (button < PMouseButton::FirstTouchID) ? 0 : int(button);

    auto iterator = m_MouseFocusMap.find(deviceID);
    if (iterator != m_MouseFocusMap.end()) {
        return ptr_tmp_cast(iterator->second);
    }
    return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PApplication::SetKeyboardFocus(Ptr<PView> view, bool focus, bool notifyServer)
{
    assert(!IsRunning() || GetMutex().IsLocked());

    if (!focus && view != m_KeyboardFocusView) {
        return;
    }

    PView* newFocus = (focus) ? ptr_raw_pointer_cast(view) : nullptr;
    PView* oldFocus = m_KeyboardFocusView;
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

Ptr<PView> PApplication::GetKeyboardFocus() const
{
    assert(!IsRunning() || GetMutex().IsLocked());

    return ptr_tmp_cast(m_KeyboardFocusView);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PApplication::CreateBitmap(int width, int height, PEColorSpace colorSpace, uint32_t flags, handle_id& outHandle, uint8_t*& inOutFramebuffer, size_t& inOutBytesPerRow)
{
    assert(!IsRunning() || GetMutex().IsLocked());

    Post<ASCreateBitmap>(m_ReplyPort.GetHandle(), width, height, colorSpace, (flags & PBitmap::CUSTOM_FRAMEBUFFER) ? inOutFramebuffer : nullptr, (flags & PBitmap::CUSTOM_FRAMEBUFFER) ? inOutBytesPerRow : 0, flags);
    Flush();

    for (;;)
    {
        MsgCreateBitmapReply reply;
        int32_t              code;
        if (m_ReplyPort.ReceiveMessage(nullptr, &code, &reply, sizeof(reply)))
        {
            if (code == PAppserverProtocol::CREATE_BITMAP_REPLY)
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
                p_system_log<PLogSeverity::ERROR>(LogCategoryGUITK, "Application::CreateBitmap() received invalid reply: {}", code);
            }
        }
        else if (get_last_error() != EINTR)
        {
            p_system_log<PLogSeverity::ERROR>(LogCategoryGUITK, "Application::CreateBitmap() receive failed: {}", strerror(get_last_error()));
            return false;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PApplication::DeleteBitmap(handle_id bitmapHandle)
{
    assert(!IsRunning() || GetMutex().IsLocked());

    Post<ASDeleteBitmap>(bitmapHandle);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PApplication::Flush()
{
    assert(!IsRunning() || GetMutex().IsLocked());

    if (m_UsedSendBufferSize > 0) {
        message_port_send(p_get_appserver_port(), m_ServerHandle, PAppserverProtocol::MESSAGE_BUNDLE, m_SendBuffer, m_UsedSendBufferSize);
        m_UsedSendBufferSize = 0;
    }    
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PApplication::Sync()
{
    assert(!IsRunning() || GetMutex().IsLocked());

    Post<ASSync>(m_ReplyPort.GetHandle());
    Flush();
    int32_t code;
    for (;;)
    {
        if (m_ReplyPort.ReceiveMessage(nullptr, &code, nullptr, 0) < 0 && get_last_error() != EINTR) break;
        if (code == PAppserverProtocol::SYNC_REPLY) {
            break;
        } else {
            p_system_log<PLogSeverity::ERROR>(LogCategoryGUITK, "Application::Sync() received invalid reply: {}", code);
        }
        
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PApplication::DetachView(Ptr<PView> view)
{
    assert(!IsRunning() || GetMutex().IsLocked());

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
    for (Ptr<PView> child : *view)
    {
        DetachView(child);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void* PApplication::AllocMessageBuffer(int32_t messageID, size_t size)
{
    assert(!IsRunning() || GetMutex().IsLocked());
    assert(size > 0);

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

bool PApplication::CreateServerView(Ptr<PView> view, handler_id parentHandle, PViewDockType dockType, size_t index)
{
    assert(!IsRunning() || GetMutex().IsLocked());

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
        , view->m_PenWidth
        , (view->m_Font != nullptr) ? view->m_Font->Get() : PFontID::e_FontLarge
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
            if (code == PAppserverProtocol::CREATE_VIEW_REPLY) {
                view->SetServerHandle(reply.m_ViewHandle);
                break;
            } else {
                p_system_log<PLogSeverity::ERROR>(LogCategoryGUITK, "Application::AddView() received invalid reply: {}", code);
            }
        }
        else if (get_last_error() != EINTR)
        {
            p_system_log<PLogSeverity::ERROR>(LogCategoryGUITK, "Application::AddView() receive failed: {}", strerror(get_last_error()));
            return false;
        }
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PApplication::RegisterViewForLayout(Ptr<PView> view, bool recursive)
{
    assert(!IsRunning() || GetMutex().IsLocked());

    if (!view->m_IsLayoutValid && !view->m_IsLayoutPending) {
        view->m_IsLayoutPending = true;
        m_ViewsNeedingLayout.insert(view);
    }
    if (recursive)
    {
        for (Ptr<PView> child : view->GetChildList())
        {
            RegisterViewForLayout(child, true);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PApplication::SetMouseDownView(PMouseButton button, Ptr<PView> view, const PMotionEvent& motionEvent)
{
    assert(!IsRunning() || GetMutex().IsLocked());

    int deviceID = (button < PMouseButton::FirstTouchID) ? 0 : int(button);

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

Ptr<PView> PApplication::GetMouseDownView(PMouseButton button) const
{
    assert(!IsRunning() || GetMutex().IsLocked());

    int deviceID = (button < PMouseButton::FirstTouchID) ? 0 : int(button);

    auto iterator = m_MouseViewMap.find(deviceID);
    if (iterator != m_MouseViewMap.end()) {
        return ptr_tmp_cast(iterator->second);
    }
    return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PApplication::LayoutViews()
{
    assert(!IsRunning() || GetMutex().IsLocked());

    std::set<Ptr<PView>> updatedViews;
    for (int i = 0; i < 100 && !m_ViewsNeedingLayout.empty(); ++i)
    {
        std::set<Ptr<PView>> list = std::move(m_ViewsNeedingLayout);
        for (Ptr<PView> view : list)
        {
            view->m_IsLayoutPending = false;
            view->RefreshLayout();
        }
        updatedViews.merge(list);
    }
    for (Ptr<PView> view : updatedViews) {
        view->OnLayoutUpdated();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PApplication::SlotLongPressTimer()
{
    assert(!IsRunning() || GetMutex().IsLocked());

    Ptr<PView> lastPressedView = GetMouseDownView(m_LastClickEvent.ButtonID);
    if (lastPressedView != nullptr) {
        lastPressedView->DispatchLongPress(m_LastClickEvent.ButtonID, lastPressedView->ConvertFromScreen(m_LastClickEvent.Position), m_LastClickEvent);
    }
}
