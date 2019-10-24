// This file is part of PadOS.
//
// Copyright (C) 2017-2018 Kurt Skauen <http://kavionic.com/>
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
// Created: 18/06/19 23:33:48

#include "sam.h"

#include "KNodeMonitor.h"

#if 0


static int g_nLastMonitorID = 1;
static sem_id g_hNodeMonitorMutex;

/*static void add_monitor(KINode* psInode, KNodeMonitorNode* psMonitor)
{
    psMonitor->nm_psInode       = psInode;
    psMonitor->nm_psNextInInode = psInode->i_psFirstMonitor;
    psInode->i_psFirstMonitor   = psMonitor;
    
    atomic_add( &psInode->i_nCount, 1 );
}

static void remove_monitor(KINode* psInode, NodeMonitorNode* psMonitor)
{
    NodeMonitorNode** ppsTmp;
    
    for ( ppsTmp = &psInode->i_psFirstMonitor ; NULL != *ppsTmp ; ppsTmp = &(*ppsTmp)->nm_psNextInInode ) {
	if ( *ppsTmp == psMonitor ) {
	    *ppsTmp = psMonitor->nm_psNextInInode;
	    break;
	}
    }
    psMonitor->nm_psInode = NULL;
    put_inode( psInode );
}*/

static int do_watch_node( bool bKernel, int nNode, int nPort, uint32_t nFlags, void* pData )
{
    KIOContext*   psCtx = get_current_iocxt( bKernel );
    Ptr<KFileTableNode> psFile;
    KNodeMonitorNode* psMonitor;
    int		   nError;
    
    psFile = FileIO::GetFileNode(nNode, bKernel);
    if (psFile == nullptr) {
	set_last_error(EMFILE);
        return -1;
    }

    psMonitor = kmalloc( sizeof( NodeMonitorNode ), MEMF_KERNEL | MEMF_CLEAR | MEMF_OKTOFAILHACK );
    if ( psMonitor == NULL ) {
	nError = -ENOMEM;
	goto error;
    }

    psMonitor->nm_psOwner     = psCtx;
    psMonitor->nm_nWatchFlags = nFlags;
    psMonitor->nm_hPortID     = nPort;
    psMonitor->nm_pUserData   = pData;
    
    LOCK( g_hNodeMonitorMutex );
    psMonitor->nm_nID = g_nLastMonitorID++;
    if ( g_nLastMonitorID < 0 ) {
	g_nLastMonitorID = 1;
    }
    add_monitor( psFile->f_psInode, psMonitor );
    psMonitor->nm_psNextInCtx = psCtx->ic_psFirstMonitor;
    psCtx->ic_psFirstMonitor  = psMonitor;
    
    UNLOCK( g_hNodeMonitorMutex );

    nError = psMonitor->nm_nID;
error:
    put_fd( psFile );
    return( nError );
}

static status_t do_delete_node_monitor( bool bKernel, int nMonitor )
{
    IoContext_s*    psCtx = get_current_iocxt( bKernel );
    KNodeMonitorNode** ppsTmp;
    KNodeMonitorNode*  psMonitor = NULL;
    int		    nError;
    
    LOCK( g_hNodeMonitorMutex );
    nError = -EINVAL;
    for ( ppsTmp = &psCtx->ic_psFirstMonitor ; NULL != *ppsTmp ; ppsTmp = &(*ppsTmp)->nm_psNextInCtx ) {
	psMonitor = *ppsTmp;
	if ( psMonitor->nm_nID == nMonitor ) {
	    *ppsTmp = psMonitor->nm_psNextInCtx;
	    nError = 0;
	    break;
	}
    }
    if ( nError == 0 ) {
	remove_monitor( psMonitor->nm_psInode, psMonitor );
    }
    UNLOCK( g_hNodeMonitorMutex );
    if ( nError == 0 ) {
	kfree( psMonitor );
    }
    return( nError );
}

int sys_watch_node( int nNode, int nPort, uint32 nFlags, void* pData )
{
    return( do_watch_node( false, nNode, nPort, nFlags, pData ) );
}

status_t sys_delete_node_monitor( int nMonitor )
{
    return( do_delete_node_monitor( false, nMonitor ) );
}


static NodeWatchEvent_s* create_full_path_event(NodeWatchEvent_s* psEvent, KINode* psInode, int nWhichPath, const char* pzName, int nNameLen)
{
    int		      nError;

    std::vector<uint8_t> eventBuffer;
    eventBuffer.resize(sizeof(NodeWatchEvent_s) + PATH_MAX);

    NodeWatchEvent_s* psFullPathEvent = (NodeWatchEvent_s*)eventBuffer.data();

    memcpy(psFullPathEvent, psEvent, psEvent->nw_nTotalSize);
    psFullPathEvent->nw_nWhichPath = nWhichPath;
    
    nError = get_dirname(psInode, NWE_PATH( psFullPathEvent ), PATH_MAX - psFullPathEvent->nw_nNameLen);

    if ( nError < 0 ) {
	kfree( psFullPathEvent );
	return( NULL );
    } else if ( nError == PATH_MAX - psFullPathEvent->nw_nNameLen ) {
	kfree( psFullPathEvent );
	return( NULL );
    } else {
	psFullPathEvent->nw_nPathLen    = nError;
	psFullPathEvent->nw_nTotalSize += psFullPathEvent->nw_nPathLen;
	return( psFullPathEvent );
    }
}

status_t KNodeMonitor::notify_node_monitors(int nEvent, dev_t nDevice, ino_t nOldDir, ino_t nNewDir, ino_t nNode, const char* pzName, int nNameLen)
{
    std::vector<uint8_t> eventBuffer;
    eventBuffer.resize(sizeof(NodeWatchEvent_s) + nNameLen);

    NodeWatchEvent_s* psEvent = (NodeWatchEvent_s*)eventBuffer.data();
    Ptr<KINode> psInode;
    Ptr<KINode> psOldDir;
    Ptr<KINode> psNewDir;
    status_t nError;

    if ( nNameLen > NAME_MAX ) {
	set_last_error(ENAMETOOLONG);
        return -1;
    }

    if ( nNode != 0 && nEvent != NWEVENT_CREATED ) {
	psInode = KVFSManager::GetINode(nDevice, nNode, false);
	if ( psInode == nullptr ) {
	    set_last_error(EIO);
	    return -1;
	}
    }
    if ( nOldDir != 0 ) {
	psOldDir = KVFSManager::GetINode(nDevice, nOldDir, false);
	if ( psOldDir == nullptr ) {
	    set_last_error(EIO);
	    return -1;
	}
    }
    if ( nNewDir != 0 ) {
	psNewDir = KVFSManager::GetINode(nDevice, nNewDir, false);
	if ( psNewDir == nullptr ) {
	    set_last_error(EIO);
	    return -1;
	}
    }

    psEvent->nw_nTotalSize = sizeof(NodeWatchEvent_s) + nNameLen;
    psEvent->nw_nEvent     = nEvent;
    psEvent->nw_nWhichPath = NWPATH_NONE;
    psEvent->nw_nDevice    = nDevice;
    psEvent->nw_nNode      = nNode;
    psEvent->nw_nOldDir    = nOldDir;
    psEvent->nw_nNewDir    = nNewDir;
    psEvent->nw_nNameLen   = nNameLen;
    memcpy( NWE_NAME(psEvent), pzName, nNameLen );
    
    CRITICAL_SCOPE(s_Mutex);
    switch( nEvent )
    {
	case NWEVENT_CREATED:
	case NWEVENT_DELETED:
	{
	    Ptr<KNodeMonitorNode> psMonitor;
	    for ( psMonitor = psOldDir->i_psFirstMonitor ; psMonitor != NULL ; psMonitor = psMonitor->nm_psNextInInode ) {
		if ( psMonitor->nm_nWatchFlags & NWATCH_DIR ) {
		    psEvent->nw_nMonitor  = psMonitor->nm_nID;
		    psEvent->nw_pUserData = psMonitor->nm_pUserData;
		    send_msg_x( psMonitor->nm_hPortID, M_NODE_MONITOR, psEvent, psEvent->nw_nTotalSize, 100000LL );
		}
	    }
	    if ( nEvent == NWEVENT_DELETED ) {
		for ( psMonitor = psInode->i_psFirstMonitor ; psMonitor != NULL ; psMonitor = psMonitor->nm_psNextInInode ) {
		    if ( psMonitor->nm_nWatchFlags & NWATCH_NAME ) {
			psEvent->nw_nMonitor  = psMonitor->nm_nID;
			psEvent->nw_pUserData = psMonitor->nm_pUserData;
			send_msg_x( psMonitor->nm_hPortID, M_NODE_MONITOR, psEvent, psEvent->nw_nTotalSize, 100000LL );
		    }
		}
	    }
	    nError = 0;
	    break;
	}   
	case NWEVENT_MOVED:
	{
	    NodeWatchEvent_s* psFullOldPathEvent = NULL;
	    NodeWatchEvent_s* psFullNewPathEvent = NULL;
	    KNodeMonitorNode*    psMonitor;
	    
	    for ( psMonitor = psOldDir->i_psFirstMonitor ; psMonitor != NULL ; psMonitor = psMonitor->nm_psNextInInode ) {
		if ( (psMonitor->nm_nWatchFlags & (NWATCH_DIR | NWATCH_FULL_DST_PATH)) == (NWATCH_DIR | NWATCH_FULL_DST_PATH) ) {
		    psFullNewPathEvent = create_full_path_event( psEvent, psNewDir, NWPATH_NEW, pzName, nNameLen );
		    break;
		}
	    }
	    for ( psMonitor = psNewDir->i_psFirstMonitor ; psMonitor != NULL ; psMonitor = psMonitor->nm_psNextInInode ) {
		if ( (psMonitor->nm_nWatchFlags & (NWATCH_DIR | NWATCH_FULL_DST_PATH)) == (NWATCH_DIR | NWATCH_FULL_DST_PATH) ) {
		    psFullOldPathEvent = create_full_path_event( psEvent, psNewDir, NWPATH_OLD, pzName, nNameLen );
		    break;
		}
	    }
	    if ( psFullNewPathEvent == NULL ) {
		for ( psMonitor = psInode->i_psFirstMonitor ; psMonitor != NULL ; psMonitor = psMonitor->nm_psNextInInode ) {
		    if ( (psMonitor->nm_nWatchFlags & (NWATCH_NAME | NWATCH_FULL_DST_PATH)) == (NWATCH_DIR | NWATCH_FULL_DST_PATH) ) {
			psFullNewPathEvent = create_full_path_event( psEvent, psNewDir, NWPATH_NEW, pzName, nNameLen );
			break;
		    }
		}
	    }
	    for ( psMonitor = psOldDir->i_psFirstMonitor ; psMonitor != NULL ; psMonitor = psMonitor->nm_psNextInInode ) {
		if ( psMonitor->nm_nWatchFlags & NWATCH_DIR ) {
		    if ( psFullNewPathEvent != NULL && psMonitor->nm_nWatchFlags & NWATCH_FULL_DST_PATH ) {
			psFullNewPathEvent->nw_nMonitor  = psMonitor->nm_nID;
			psFullNewPathEvent->nw_pUserData = psMonitor->nm_pUserData;
			send_msg_x( psMonitor->nm_hPortID, M_NODE_MONITOR, psFullNewPathEvent, psFullNewPathEvent->nw_nTotalSize, 100000LL );
		    } else {
			psEvent->nw_nMonitor  = psMonitor->nm_nID;
			psEvent->nw_pUserData = psMonitor->nm_pUserData;
			send_msg_x( psMonitor->nm_hPortID, M_NODE_MONITOR, psEvent, psEvent->nw_nTotalSize, 100000LL );
		    }
		}
	    }
	    if ( psNewDir != psOldDir ) {
		for ( psMonitor = psNewDir->i_psFirstMonitor ; psMonitor != NULL ; psMonitor = psMonitor->nm_psNextInInode ) {
		    if ( psMonitor->nm_nWatchFlags & NWATCH_DIR ) {
			if ( psFullOldPathEvent != NULL && psMonitor->nm_nWatchFlags & NWATCH_FULL_DST_PATH ) {
			    psFullOldPathEvent->nw_nMonitor  = psMonitor->nm_nID;
			    psFullOldPathEvent->nw_pUserData = psMonitor->nm_pUserData;
			    send_msg_x( psMonitor->nm_hPortID, M_NODE_MONITOR, psFullOldPathEvent, psFullOldPathEvent->nw_nTotalSize, 100000LL );
			} else {
			    psEvent->nw_nMonitor  = psMonitor->nm_nID;
			    psEvent->nw_pUserData = psMonitor->nm_pUserData;
			    send_msg_x( psMonitor->nm_hPortID, M_NODE_MONITOR, psEvent, psEvent->nw_nTotalSize, 100000LL );
			}
		    }
		}
	    }
	    for ( psMonitor = psInode->i_psFirstMonitor ; psMonitor != NULL ; psMonitor = psMonitor->nm_psNextInInode ) {
		if ( psMonitor->nm_nWatchFlags & NWATCH_NAME ) {
		    if ( psFullNewPathEvent != NULL && psMonitor->nm_nWatchFlags & NWATCH_FULL_DST_PATH ) {
			psFullNewPathEvent->nw_nMonitor  = psMonitor->nm_nID;
			psFullNewPathEvent->nw_pUserData = psMonitor->nm_pUserData;
			send_msg_x( psMonitor->nm_hPortID, M_NODE_MONITOR, psFullNewPathEvent, psFullNewPathEvent->nw_nTotalSize, 100000LL );
		    } else {
			psEvent->nw_nMonitor  = psMonitor->nm_nID;
			psEvent->nw_pUserData = psMonitor->nm_pUserData;
			send_msg_x( psMonitor->nm_hPortID, M_NODE_MONITOR, psEvent, psEvent->nw_nTotalSize, 100000LL );
		    }
		}
	    }
	    if ( psFullOldPathEvent != NULL ) {
		kfree( psFullOldPathEvent );
	    }
	    if ( psFullNewPathEvent != NULL ) {
		kfree( psFullNewPathEvent );
	    }
	    nError = 0;
	    break;
	}   
	case NWEVENT_STAT_CHANGED:
	{
	    KNodeMonitorNode* psMonitor;

	    for ( psMonitor = psInode->i_psFirstMonitor ; psMonitor != NULL ; psMonitor = psMonitor->nm_psNextInInode ) {
		if ( psMonitor->nm_nWatchFlags & NWATCH_STAT ) {
		    psEvent->nw_nMonitor  = psMonitor->nm_nID;
		    psEvent->nw_pUserData = psMonitor->nm_pUserData;
		    send_msg_x( psMonitor->nm_hPortID, M_NODE_MONITOR, psEvent, psEvent->nw_nTotalSize, 100000LL );
		}
	    }
	    nError = 0;
	    break;
	}
	case NWEVENT_ATTR_WRITTEN:
	case NWEVENT_ATTR_DELETED:
	{
	    KNodeMonitorNode* psMonitor;

	    for ( psMonitor = psInode->i_psFirstMonitor ; psMonitor != NULL ; psMonitor = psMonitor->nm_psNextInInode ) {
		if ( psMonitor->nm_nWatchFlags & NWATCH_ATTR ) {
		    psEvent->nw_nMonitor  = psMonitor->nm_nID;
		    psEvent->nw_pUserData = psMonitor->nm_pUserData;
		    send_msg_x( psMonitor->nm_hPortID, M_NODE_MONITOR, psEvent, psEvent->nw_nTotalSize, 100000LL );
		}
	    }
	    nError = 0;
	    break;
	}
	case NWEVENT_FS_MOUNTED:
	case NWEVENT_FS_UNMOUNTED:
	default:
	    nError = -EINVAL;
	    break;
    }
    UNLOCK( g_hNodeMonitorMutex );
error:
    if ( psInode != NULL ) {
	put_inode( psInode );
    }
    if ( psOldDir != NULL ) {
	put_inode( psOldDir );
    }
    if ( psNewDir != NULL ) {
	put_inode( psNewDir );
    }
    return( nError );
}

void nwatch_exit( IoContext_s* psCtx )
{
    LOCK( g_hNodeMonitorMutex );
    while ( psCtx->ic_psFirstMonitor != NULL )
    {
	KNodeMonitorNode* psMonitor = psCtx->ic_psFirstMonitor;
	psCtx->ic_psFirstMonitor = psMonitor->nm_psNextInCtx;
	
	remove_monitor( psMonitor->nm_psInode, psMonitor );
	kfree( psMonitor );
    }
    UNLOCK( g_hNodeMonitorMutex );
}



status_t nwatch_init( void )
{
    g_hNodeMonitorMutex = create_semaphore( "node_monitor", 1, SEM_REQURSIVE );
    if ( g_hNodeMonitorMutex < 0 ) {
	return( g_hNodeMonitorMutex );
    }
    return( 0 );
}

#endif