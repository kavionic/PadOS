// This file is part of PadOS.
//
// Copyright (C) 2026 Kurt Skauen <http://kavionic.com/>
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
// Created: 06.03.2026 20:00

#include <Kernel/Scheduler.h>
#include <Kernel/KProcessGroups.h>

namespace kernel
{


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int ksetsid_trw()
{
    KProcess& thisProc = kget_current_process();

    if (thisProc.IsGroupLeader()) {
        PERROR_THROW_CODE(PErrorCode::NoPermission);
    }

    const pid_t pgroup = thisProc.GetPID();

    Ptr<KInode> controllingTTY;
    {
        KProcessLock processLock;

        // Make sure the process group we are about to make don't already exist.
        for (auto it : KProcess::GetProcessMap())
        {
            const KProcess* const process = it.second;

            if (process->GetPGroupID() == pgroup) {
                PERROR_THROW_CODE(PErrorCode::NoPermission);
            }
        }

        thisProc.SetSession(pgroup);
        thisProc.SetPGroupID(pgroup);
        controllingTTY = thisProc.GetIOContext().GetControllingTTY();
        thisProc.GetIOContext().SetControllingTTY(nullptr);
    }
    return pgroup;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ksetpgid_trw(pid_t inDest, pid_t inGroup)
{
    const KProcess& thisProcess = kget_current_process();

    pid_t dstPID = (0 == inDest) ? thisProcess.GetPID() : inDest;
    pid_t pgroup = (0 == inGroup) ? dstPID : inGroup;

    if (pgroup <= 0) {
        PERROR_THROW_CODE(PErrorCode::InvalidArg);
    }

    Ptr<KProcess> dstProcess = KProcess::GetProcess(dstPID);

    if (dstProcess == nullptr)
    {
        kernel_log<PLogSeverity::NOTICE>(LogCatKernel_Processes, "{}: dest process {} not found.", __PRETTY_FUNCTION__, dstPID);
        PERROR_THROW_CODE(PErrorCode::NoSuchProcess);
    }

    if (dstProcess->GetParentPID() == thisProcess.GetPID())
    {
        if (dstProcess->HasExeced()) {
            PERROR_THROW_CODE(PErrorCode::NoAccess);
        }
        if (dstProcess->GetSession() != thisProcess.GetSession()) {
            PERROR_THROW_CODE(PErrorCode::NoPermission);
        }
    }
    else
    {
        if (dstProcess != &thisProcess)
        {
            kernel_log<PLogSeverity::NOTICE>(LogCatKernel_Processes, "{}: {} not same as {}\n", __PRETTY_FUNCTION__, dstProcess->GetPID(), thisProcess.GetPID());
            PERROR_THROW_CODE(PErrorCode::NoSuchProcess);
        }
    }

    if (dstProcess->IsGroupLeader()) {
        PERROR_THROW_CODE(PErrorCode::NoPermission);
    }
    if (pgroup != dstPID)
    {
        bool	groupOK = false;

        KProcessLock processLock;

        // Make sure the process group we are about to make don't already exist.
        for (auto it : KProcess::GetProcessMap())
        {
            const KProcess* const process = it.second;

            if (process->GetPGroupID() == pgroup && process->GetSession() == thisProcess.GetSession())
            {
                groupOK = true;
                break;
            }
        }

        if (!groupOK) {
            PERROR_THROW_CODE(PErrorCode::NoPermission);
        }
    }
    dstProcess->SetPGroupID(pgroup);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

pid_t kgetpgrp()
{
    return kget_current_process().GetPGroupID();
}


} // namespace kernel
