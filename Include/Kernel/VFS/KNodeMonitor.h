// This file is part of PadOS.
//
// Copyright (c) 2017-2018 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 18/06/19 23:33:48

#pragma once
#include <sys/types.h>
#include "Ptr/PtrTarget.h"
#include "Ptr/Ptr.h"
#include "Kernel/KMutex.h"

namespace kernel
{
    
class KIOContext;    
class KInode;


enum
{
    NWATCH_NAME	 	 = 0x0001,
    NWATCH_STAT  	 = 0x0002,
    NWATCH_ATTR  	 = 0x0004,
    NWATCH_DIR   	 = 0x0008,
    NWATCH_ALL   	 = 0x000f,
    NWATCH_FULL_DST_PATH = 0x0010,
    NWATCH_MOUNT 	 = 0x0100,
    NWATCH_WORLD 	 = 0x0200
};

enum
{
    NWEVENT_CREATED = 1,
    NWEVENT_DELETED,
    NWEVENT_MOVED,
    NWEVENT_STAT_CHANGED,
    NWEVENT_ATTR_WRITTEN,
    NWEVENT_ATTR_DELETED,
    NWEVENT_FS_MOUNTED,
    NWEVENT_FS_UNMOUNTED
};

enum
{
    NWPATH_NONE,
    NWPATH_OLD,
    NWPATH_NEW
};

struct NodeWatchEvent_s
{
    int		nw_nTotalSize;
    int		nw_nEvent;
    int		nw_nWhichPath;
    int		nw_nMonitor;
    void*	nw_pUserData;
    dev_t	nw_nDevice;
    ino_t	nw_nNode;
    ino_t	nw_nOldDir;
    ino_t	nw_nNewDir;
    int		nw_nNameLen;
    int		nw_nPathLen;
    int         sequence_number;
};

#define NWE_NAME( event ) ((char*)((event)+1))
#define NWE_PATH( event ) (NWE_NAME( (event ) ) + (event)->nw_nNameLen)

class KNodeMonitorNode : public PtrTarget
{
public:
//    KNodeMonitorNode* nm_psNextInInode;
//    KNodeMonitorNode* nm_psNextInCtx;
    int		nm_nID;
    void*	nm_pUserData;
    KIOContext* nm_psOwner;
    Ptr<KInode>	nm_psInode;
    uint32_t	nm_nWatchFlags;
    port_id	nm_hPortID;
};

class KNodeMonitor
{
public:
    KNodeMonitor();
    ~KNodeMonitor();

    static status_t notify_node_monitors(int nEvent, dev_t nDevice, ino_t nOldDir, ino_t nNewDir, ino_t nNode, const char* pzName, int nNameLen);

private:
    static KMutex s_Mutex;
    KNodeMonitor(const KNodeMonitor&) = delete;
    KNodeMonitor& operator=(const KNodeMonitor&) = delete;
};

} // namespace
