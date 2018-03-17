/* 
* KFileHandle.h
*
* Created: 23.02.2018 01:47:38
* Author: Kurt
*/


#ifndef __KFILEHANDLE_H__
#define __KFILEHANDLE_H__

#include <sys/types.h>
#include <stddef.h>

#include "GUIToolkit/PtrTarget.h"
#include "GUIToolkit/Ptr.h"

class KINode;

class KFileHandle : public PtrTarget
{
public:
    KFileHandle(Ptr<KINode> inode);
    Ptr<KINode> m_INode;
    int   m_FileMode = 0;
    off_t m_Position = 0;
};

class KDirectoryHandle : public KFileHandle
{
    public:
    KDirectoryHandle(Ptr<KINode> inode) : KFileHandle(inode) {}
};

#endif //__KFILEHANDLE_H__
