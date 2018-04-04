// This file is part of PadOS.
//
// Copyright (C) 2018 Kurt Skauen <http://kavionic.com/>
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
// Created: 23.02.2018 01:41:28

#include <assert.h>
#include <string.h>

#include "SystemSetup.h"

#include "Kernel.h"
#include "Scheduler.h"
#include "VFS/KDeviceNode.h"
#include "VFS/KRootFilesystem.h"
#include "VFS/KFSVolume.h"
#include "VFS/KFileHandle.h"
#include "System/Utils/Utils.h"
#include "HAL/SAME70System.h"

using namespace kernel;

volatile bigtime_t            Kernel::s_SystemTime = 0;
//int                           Kernel::s_LastError = 0;
Ptr<KFileHandle>              Kernel::s_PlaceholderFile;
Ptr<KRootFilesystem>          Kernel::s_RootFilesystem;
Ptr<KFSVolume>                Kernel::s_RootVolume;
std::vector<Ptr<KFileHandle>> Kernel::s_FileTable;
KIRQAction*                   Kernel::s_IRQHandlers[PERIPH_COUNT_IRQn];

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void kernel::panic(const char* message)
{
    RGBLED_R.Write(true);
    RGBLED_G.Write(false);
    RGBLED_B.Write(false);

//    write(1, message, strlen(message));
    volatile bool freeze = true;
    while(freeze);
    RGBLED_R.Write(false);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bigtime_t get_system_time()
{
    return Kernel::GetTime();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bigtime_t get_system_time_hires()
{
    for (;;)
    {
        uint32_t ticks = SysTick->VAL;
        bigtime_t time = Kernel::GetTime();
        if (SysTick->VAL <= ticks) {
            return time + ((CLOCK_CPU_FREQUENCY / 1000 - 1) - ticks) * 1000 / (CLOCK_CPU_FREQUENCY / 1000);
        }
    }    
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Kernel::Initialize()
{
    s_PlaceholderFile = ptr_new<KFileHandle>();
    s_RootFilesystem = ptr_new<KRootFilesystem>();
    s_RootVolume =  s_RootFilesystem->Mount(nullptr, "", 0);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Kernel::SystemTick()
{
    s_SystemTime++;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int Kernel::RegisterDevice(const char* path, Ptr<KDeviceNode> device)
{
    s_RootFilesystem->RegisterDevice(path, device);
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int Kernel::RegisterIRQHandler(IRQn_Type irqNum, KIRQHandler* handler, void* userData)
{
    if (irqNum < 0 || irqNum >= PERIPH_COUNT_IRQn || handler == nullptr)
    {
        set_last_error(EINVAL);
        return -1;
    }
    KIRQAction* action = new KIRQAction;
    if (action == nullptr) {
        set_last_error(ENOMEM);
        return -1;
    }
    static int currentHandle = 0;
    int handle = ++currentHandle;

    action->m_Handle = handle;
    action->m_Handler = handler;
    action->m_UserData = userData;

    CRITICAL_BEGIN(CRITICAL_IRQ)
    {
        bool needEnabled = s_IRQHandlers[irqNum] == nullptr;
        
        action->m_Next = s_IRQHandlers[irqNum];
        s_IRQHandlers[irqNum] = action;

        if (needEnabled) {
            NVIC_SetPriority(irqNum, KIRQ_PRI_NORMAL_LATENCY2);
            NVIC_EnableIRQ(irqNum);
        }
    } CRITICAL_END;
    return handle;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int Kernel::UnregisterIRQHandler(IRQn_Type irqNum, int handle)
{
    if (irqNum < 0 || irqNum >= PERIPH_COUNT_IRQn)
    {
        set_last_error(EINVAL);
        return -1;
    }
    CRITICAL_BEGIN(CRITICAL_IRQ)
    {
        KIRQAction* prev = nullptr;
        for (KIRQAction* action = s_IRQHandlers[irqNum]; action != nullptr; action = action->m_Next)
        {
            if (action->m_Handle == handle)
            {
                if (prev != nullptr) {
                    prev->m_Next = action->m_Next;
                } else {
                    s_IRQHandlers[irqNum] = action->m_Next;
                }
                delete action;
                if (s_IRQHandlers[irqNum] == nullptr) {
                    NVIC_DisableIRQ(irqNum);
                }
                return 0;
            }
        }
    } CRITICAL_END;
    set_last_error(EINVAL);
    return -1;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Kernel::HandleIRQ(IRQn_Type irqNum)
{
    for (KIRQAction* action = s_IRQHandlers[irqNum]; action != nullptr; action = action->m_Next) {
        action->m_Handler(irqNum, action->m_UserData);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bigtime_t Kernel::GetTime()
{
    for (;;)
    {
        volatile uint32_t* timer = reinterpret_cast<volatile uint32_t*>(&s_SystemTime);

#ifdef LITTLE_ENDIAN
        uint32_t time1LO = timer[0];
        __DMB();
        uint32_t time1HI = timer[1];
        __DMB();
        uint32_t time2LO = timer[0];
        // Make sure we re-fetch s_SystemTime if the previous increment caused
        // a wrapping, and we got interrupted after reading the first word.
        if (time1LO <= time2LO) {
            uint64_t result;
            uint32_t* resultPtr = reinterpret_cast<uint32_t*>(&result);
            resultPtr[0] = time1LO;
            resultPtr[1] = time1HI;
            return result * 1000;
        }
#else // LITTLE_ENDIAN
#error
#endif // LITTLE_ENDIAN
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int Kernel::OpenFile(const char* path, int flags)
{
    if (path[0] == '/') path++;
    int handle = AllocateFileHandle();
    if (handle < 0) {
        return handle;
    }
    Ptr<KINode> inode = s_RootFilesystem->LocateInode(s_RootVolume->m_RootNode, path, strlen(path));
    if (inode == nullptr)
    {
        FreeFileHandle(handle);
        return -1;
    }
    Ptr<KFileHandle> file =s_RootFilesystem->OpenFile(inode, flags);
    if (file == nullptr)
    {
        s_RootFilesystem->ReleaseInode(inode);
        FreeFileHandle(handle);
        return -1;
    }
    SetFile(handle, file);
    return handle;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int Kernel::DupeFile(int oldHandle, int newHandle)
{
    if (oldHandle == newHandle)
    {
        set_last_error(EINVAL);
        return -1;
    }
    Ptr<KFileHandle> file = GetFile(oldHandle);
    if (file == nullptr)
    {
        set_last_error(EBADF);
        return -1;
    }
    if (newHandle == -1) {
        newHandle = AllocateFileHandle();
    } else {
        CloseFile(newHandle);
        if (newHandle >= int(s_FileTable.size())) {
            s_FileTable.resize(newHandle + 1);
        }
    }
    if (newHandle >= 0) {
        SetFile(newHandle, file);
    } else {
        set_last_error(EMFILE);
    }
    return newHandle;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int Kernel::CloseFile(int handle)
{
    int result = 0;

    Ptr<KFileHandle> file = GetFile(handle);
    if (file == nullptr)
    {
        set_last_error(EBADF);
        return -1;
    }
    FreeFileHandle(handle);

    Ptr<KINode> inode = file->m_INode;
    assert(inode != nullptr && inode->m_Filesystem != nullptr);

    if (inode->m_Filesystem->CloseFile(file) < 0)
    {
        result = -1;
    }
    inode->m_Filesystem->ReleaseInode(inode);

    return result;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ssize_t Kernel::Read(int handle, void* buffer, size_t length)
{
    Ptr<KFileHandle> file = GetFile(handle);
    if (file == nullptr)
    {
        set_last_error(EBADF);
        return -1;
    }
    Ptr<KINode> inode = file->m_INode;
    assert(inode != nullptr && inode->m_Filesystem != nullptr);

    ssize_t result = inode->m_Filesystem->Read(file, file->m_Position, buffer, length);
    if (result < 0)
    {
        return result;
    }
    file->m_Position += result;
    return result;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ssize_t Kernel::Write(int handle, const void* buffer, size_t length)
{
    Ptr<KFileHandle> file = GetFile(handle);
    if (file == nullptr)
    {
        set_last_error(EBADF);
        return -1;
    }
    Ptr<KINode> inode = file->m_INode;
    assert(inode != nullptr && inode->m_Filesystem != nullptr);

    ssize_t result = inode->m_Filesystem->Write(file, file->m_Position, buffer, length);
    if (result < 0)
    {
        return result;
    }
    file->m_Position += result;
    return result;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ssize_t Kernel::Read(int handle, off_t position, void* buffer, size_t length)
{
    Ptr<KFileHandle> file = GetFile(handle);
    if (file == nullptr)
    {
        set_last_error(EBADF);
        return -1;
    }
    Ptr<KINode> inode = file->m_INode;
    assert(inode != nullptr && inode->m_Filesystem != nullptr);

    return inode->m_Filesystem->Read(file, position, buffer, length);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ssize_t Kernel::Write(int handle, off_t position, const void* buffer, size_t length)
{
    Ptr<KFileHandle> file = GetFile(handle);
    if (file == nullptr)
    {
        set_last_error(EBADF);
        return -1;
    }
    Ptr<KINode> inode = file->m_INode;
    assert(inode != nullptr && inode->m_Filesystem != nullptr);
    return inode->m_Filesystem->Write(file, position, buffer, length);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int Kernel::DeviceControl(int handle, int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength)
{
    Ptr<KFileHandle> file = GetFile(handle);
    if (file == nullptr)
    {
        set_last_error(EBADF);
        return -1;
    }
    Ptr<KINode> inode = file->m_INode;
    assert(inode != nullptr && inode->m_Filesystem != nullptr);
    return inode->m_Filesystem->DeviceControl(file, request, inData, inDataLength, outData, outDataLength);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int Kernel::ReadAsync(int handle, off_t position, void* buffer, size_t length, void* userObject, AsyncIOResultCallback* callback)
{
    Ptr<KFileHandle> file = GetFile(handle);
    if (file == nullptr)
    {
        set_last_error(EBADF);
        return -1;
    }
    Ptr<KINode> inode = file->m_INode;
    assert(inode != nullptr && inode->m_Filesystem != nullptr);
    return inode->m_Filesystem->ReadAsync(file, position, buffer, length, userObject, callback);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int Kernel::WriteAsync(int handle, off_t position, const void* buffer, size_t length, void* userObject, AsyncIOResultCallback* callback)
{
    Ptr<KFileHandle> file = GetFile(handle);
    if (file == nullptr)
    {
        set_last_error(EBADF);
        return -1;
    }
    Ptr<KINode> inode = file->m_INode;
    assert(inode != nullptr && inode->m_Filesystem != nullptr);
    return inode->m_Filesystem->WriteAsync(file, position, buffer, length, userObject, callback);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int Kernel::CancelAsyncRequest(int handle, int requestHandle)
{
    Ptr<KFileHandle> file = GetFile(handle);
    if (file == nullptr)
    {
        set_last_error(EBADF);
        return -1;
    }
    Ptr<KINode> inode = file->m_INode;
    assert(inode != nullptr && inode->m_Filesystem != nullptr);
    return inode->m_Filesystem->CancelAsyncRequest(file, requestHandle);    
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int Kernel::AllocateFileHandle()
{
    auto i = std::find(s_FileTable.begin(), s_FileTable.end(), nullptr);
    if (i != s_FileTable.end())
    {
        int file = i - s_FileTable.begin();
        s_FileTable[file] = s_PlaceholderFile;
        return file;
    }
    else
    {
        int file = s_FileTable.size();
        s_FileTable.push_back(s_PlaceholderFile);
        return file;
        //set_last_error(EMFILE);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Kernel::FreeFileHandle(int handle)
{
    if (handle >= 0 && handle < int(s_FileTable.size())) {
        s_FileTable[handle] = nullptr;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KFileHandle> Kernel::GetFile(int handle)
{
    if (handle >= 0 && handle < int(s_FileTable.size()) && s_FileTable[handle] != nullptr && s_FileTable[handle]->m_INode != nullptr)
    {
        return s_FileTable[handle];
    }
    return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Kernel::SetFile(int handle, Ptr<KFileHandle> file)
{
    if (handle >= 0 && handle < int(s_FileTable.size()))
    {
        s_FileTable[handle] = file;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int get_last_error()
{
    return gk_CurrentThread->m_NewLibreent._errno;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void set_last_error(int error)
{
    gk_CurrentThread->m_NewLibreent._errno = error;
}


void SUPC_Handler         ( void ) { Kernel::HandleIRQ(SUPC_IRQn); }
void RSTC_Handler         ( void ) { Kernel::HandleIRQ(RSTC_IRQn); }
void RTC_Handler          ( void ) { Kernel::HandleIRQ(RTC_IRQn); }
void RTT_Handler          ( void ) { Kernel::HandleIRQ(RTT_IRQn); }
void WDT_Handler          ( void ) { Kernel::HandleIRQ(WDT_IRQn); }
void PMC_Handler          ( void ) { Kernel::HandleIRQ(PMC_IRQn); }
void EFC_Handler          ( void ) { Kernel::HandleIRQ(EFC_IRQn); }
//void UART0_Handler        ( void ) { Kernel::HandleIRQ(UART0_IRQn); }
//void UART1_Handler        ( void ) { Kernel::HandleIRQ(UART1_IRQn); }
void PIOA_Handler         ( void ) { Kernel::HandleIRQ(PIOA_IRQn); }
void PIOB_Handler         ( void ) { Kernel::HandleIRQ(PIOB_IRQn); }
void PIOC_Handler         ( void ) { Kernel::HandleIRQ(PIOC_IRQn); }
void USART0_Handler       ( void ) { Kernel::HandleIRQ(USART0_IRQn); }
void USART1_Handler       ( void ) { Kernel::HandleIRQ(USART1_IRQn); }
void USART2_Handler       ( void ) { Kernel::HandleIRQ(USART2_IRQn); }
void PIOD_Handler         ( void ) { Kernel::HandleIRQ(PIOD_IRQn); }
void PIOE_Handler         ( void ) { Kernel::HandleIRQ(PIOE_IRQn); }
void HSMCI_Handler        ( void ) { Kernel::HandleIRQ(HSMCI_IRQn); }
void TWIHS0_Handler       ( void ) { Kernel::HandleIRQ(TWIHS0_IRQn); }
void TWIHS1_Handler       ( void ) { Kernel::HandleIRQ(TWIHS1_IRQn); }
void SPI0_Handler         ( void ) { Kernel::HandleIRQ(SPI0_IRQn); }
void SSC_Handler          ( void ) { Kernel::HandleIRQ(SSC_IRQn); }
void TC0_Handler          ( void ) { Kernel::HandleIRQ(TC0_IRQn); }
void TC1_Handler          ( void ) { Kernel::HandleIRQ(TC1_IRQn); }
void TC2_Handler          ( void ) { Kernel::HandleIRQ(TC2_IRQn); }
void TC3_Handler          ( void ) { Kernel::HandleIRQ(TC3_IRQn); }
void TC4_Handler          ( void ) { Kernel::HandleIRQ(TC4_IRQn); }
void TC5_Handler          ( void ) { Kernel::HandleIRQ(TC5_IRQn); }
void AFEC0_Handler        ( void ) { Kernel::HandleIRQ(AFEC0_IRQn); }
void DACC_Handler         ( void ) { Kernel::HandleIRQ(DACC_IRQn); }
void PWM0_Handler         ( void ) { Kernel::HandleIRQ(PWM0_IRQn); }
void ICM_Handler          ( void ) { Kernel::HandleIRQ(ICM_IRQn); }
void ACC_Handler          ( void ) { Kernel::HandleIRQ(ACC_IRQn); }
void USBHS_Handler        ( void ) { Kernel::HandleIRQ(USBHS_IRQn); }
void MCAN0_Handler        ( void ) { Kernel::HandleIRQ(MCAN0_IRQn); }
void MCAN1_Handler        ( void ) { Kernel::HandleIRQ(MCAN1_IRQn); }
void GMAC_Handler         ( void ) { Kernel::HandleIRQ(GMAC_IRQn); }
void AFEC1_Handler        ( void ) { Kernel::HandleIRQ(AFEC1_IRQn); }
void TWIHS2_Handler       ( void ) { Kernel::HandleIRQ(TWIHS2_IRQn); }
void SPI1_Handler         ( void ) { Kernel::HandleIRQ(SPI1_IRQn); }
void QSPI_Handler         ( void ) { Kernel::HandleIRQ(QSPI_IRQn); }
//void UART2_Handler        ( void ) { Kernel::HandleIRQ(UART2_IRQn); }
//void UART3_Handler        ( void ) { Kernel::HandleIRQ(UART3_IRQn); }
//void UART4_Handler        ( void ) { Kernel::HandleIRQ(UART4_IRQn); }
void TC6_Handler          ( void ) { Kernel::HandleIRQ(TC6_IRQn); }
void TC7_Handler          ( void ) { Kernel::HandleIRQ(TC7_IRQn); }
void TC8_Handler          ( void ) { Kernel::HandleIRQ(TC8_IRQn); }
void TC9_Handler          ( void ) { Kernel::HandleIRQ(TC9_IRQn); }
void TC10_Handler         ( void ) { Kernel::HandleIRQ(TC10_IRQn); }
void TC11_Handler         ( void ) { Kernel::HandleIRQ(TC11_IRQn); }
void AES_Handler          ( void ) { Kernel::HandleIRQ(AES_IRQn); }
void TRNG_Handler         ( void ) { Kernel::HandleIRQ(TRNG_IRQn); }
void XDMAC_Handler        ( void ) { Kernel::HandleIRQ(XDMAC_IRQn); }
void ISI_Handler          ( void ) { Kernel::HandleIRQ(ISI_IRQn); }
void PWM1_Handler         ( void ) { Kernel::HandleIRQ(PWM1_IRQn); }
void SDRAMC_Handler       ( void ) { Kernel::HandleIRQ(SDRAMC_IRQn); }
void RSWDT_Handler        ( void ) { Kernel::HandleIRQ(RSWDT_IRQn); }
