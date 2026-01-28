// This file is part of PadOS.
//
// Copyright (C) 2001-2020 Kurt Skauen <http://kavionic.com/>
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

#include <unistd.h>
#include <fcntl.h>

#include <Storage/NodeMonitor.h>
#include <Storage/FileReference.h>
#include <Storage/FSNode.h>
#include <Storage/Directory.h>

using namespace os;

#if 0


int NodeMonitor::_CreateMonitor(const PString& cPath, uint32_t nFlags, const Messenger& cTarget)
{
    int nFile = open( cPath.c_str(), O_RDONLY );
    if ( nFile < 0 ) {
	return( -1 );
    }
    int nMonitor = watch_node( nFile, cTarget.m_hPort, nFlags, (void*)cTarget.m_hHandlerID );
    close( nFile );
    return( nMonitor );
}

int NodeMonitor::_CreateMonitor(const Directory& cDir, const PString& cPath, uint32_t nFlags, const Messenger& cTarget)
{
    if ( cDir.IsValid() == false ) {
	return( -1 );
    }
    int nFile = based_open( cDir.GetFileDescriptor(), cPath.c_str(), O_RDONLY, nFlags );
    if ( nFile < 0 ) {
	return( -1 );
    }
    int nMonitor = watch_node( nFile, cTarget.m_hPort, nFlags, (void*)cTarget.m_hHandlerID );
    close( nFile );
    return( nMonitor );
}

int NodeMonitor::_CreateMonitor( const FileReference& cRef, uint32_t nFlags, const Messenger& cTarget )
{
    if ( cRef.IsValid() == false ) {
	return( -1 );
    }
    int nFile = based_open( cRef.GetDirectory().GetFileDescriptor(), cRef.GetName().c_str(), O_RDONLY );
    if ( nFile < 0 ) {
	return( -1 );
    }
    int nMonitor = watch_node( nFile, cTarget.m_hPort, nFlags, (void*)cTarget.m_hHandlerID );
    close( nFile );
    return( nMonitor );
}

int NodeMonitor::_CreateMonitor( const FSNode* pcNode, uint32_t nFlags, const Messenger& cTarget )
{
    if ( pcNode->IsValid() == false ) {
	errno = EINVAL;
	return( -1 );
    }
    int nMonitor = watch_node( pcNode->GetFileDescriptor(), cTarget.m_hPort, nFlags, (void*)cTarget.m_hHandlerID );
    return( nMonitor );
}



NodeMonitor::NodeMonitor()
{
    m_nMonitor = -1;
}

NodeMonitor::NodeMonitor(const PString& cPath, uint32_t nFlags, const Handler* pcHandler, const Looper* pcLooper)
{
    m_nMonitor = _CreateMonitor( cPath, nFlags, Messenger( pcHandler, pcLooper ) );
    if ( m_nMonitor < 0 ) {
	throw errno_exception( "Failed to create monitor" );
    }
}

NodeMonitor::NodeMonitor(const PString& cPath, uint32_t nFlags, const Messenger& cTarget)
{
    m_nMonitor = _CreateMonitor( cPath, nFlags, cTarget );
    if ( m_nMonitor < 0 ) {
	throw errno_exception( "Failed to create monitor" );
    }
}

NodeMonitor::NodeMonitor(const Directory& cDir, const PString& cPath, uint32_t nFlags, const Handler* pcHandler, const Looper* pcLooper)
{
    if ( cDir.IsValid() == false ) {
	throw errno_exception( "Invalid directory", EINVAL );
    }
    m_nMonitor = _CreateMonitor( cDir, cPath, nFlags, Messenger( pcHandler, pcLooper ) );
    if ( m_nMonitor < 0 ) {
	throw errno_exception( "Failed to create monitor" );
    }
}

NodeMonitor::NodeMonitor(const Directory& cDir, const PString& cPath, uint32_t nFlags, const Messenger& cTarget)
{
    if ( cDir.IsValid() == false ) {
	throw errno_exception( "Invalid directory", EINVAL );
    }
    m_nMonitor = _CreateMonitor( cDir, cPath, nFlags, cTarget );
    if ( m_nMonitor < 0 ) {
	throw errno_exception( "Failed to create monitor" );
    }
}

NodeMonitor::NodeMonitor( const FileReference& cRef, uint32_t nFlags, const Handler* pcHandler, const Looper* pcLooper )
{
    if ( cRef.IsValid() == false ) {
	throw errno_exception( "Invalid directory", EINVAL );
    }
    m_nMonitor = _CreateMonitor( cRef, nFlags, Messenger( pcHandler, pcLooper ) );
    if ( m_nMonitor < 0 ) {
	throw errno_exception( "Failed to create monitor" );
    }
}

NodeMonitor::NodeMonitor( const FileReference& cRef, uint32_t nFlags, const Messenger& cTarget )
{
    if ( cRef.IsValid() == false ) {
	throw errno_exception( "Invalid directory", EINVAL );
    }
    m_nMonitor = _CreateMonitor( cRef, nFlags, cTarget );
    if ( m_nMonitor < 0 ) {
	throw errno_exception( "Failed to create monitor" );
    }
}

NodeMonitor::NodeMonitor( const FSNode* pcNode, uint32_t nFlags, const Handler* pcHandler, const Looper* pcLooper )
{
    if ( pcNode->IsValid() == false ) {
	throw errno_exception( "Invalid node", EINVAL );
    }
    m_nMonitor = _CreateMonitor( pcNode, nFlags, Messenger( pcHandler, pcLooper ) );
    if ( m_nMonitor < 0 ) {
	throw errno_exception( "Failed to create monitor" );
    }
}

NodeMonitor::NodeMonitor( const FSNode* pcNode, uint32_t nFlags, const Messenger& cTarget )
{
    if ( pcNode->IsValid() == false ) {
	throw errno_exception( "Invalid node", EINVAL );
    }
    m_nMonitor = _CreateMonitor( pcNode, nFlags, cTarget );
    if ( m_nMonitor < 0 ) {
	throw errno_exception( "Failed to create monitor" );
    }
}

NodeMonitor::~NodeMonitor()
{
    if ( m_nMonitor >= 0 ) {
	delete_node_monitor( m_nMonitor );
    }
}

status_t NodeMonitor::Unset()
{
    if ( m_nMonitor >= 0 ) {
	return( delete_node_monitor( m_nMonitor ) );
    } else {
	errno = EINVAL;
	return( -1 );
    }
    
}

status_t NodeMonitor::SetTo(const PString& cPath, uint32_t nFlags, const Messenger& cTarget)
{
    int nNewMonitor = _CreateMonitor( cPath, nFlags, cTarget );
    if ( nNewMonitor < 0 ) {
	return( -1 );
    }
    if ( m_nMonitor >= 0 ) {
	delete_node_monitor( m_nMonitor );
    }
    m_nMonitor = nNewMonitor;
    return( 0 );
}

status_t NodeMonitor::SetTo(const PString& cPath, uint32_t nFlags, const Handler* pcHandler, const Looper* pcLooper)
{
    return( SetTo( cPath, nFlags, Messenger( pcHandler, pcLooper ) ) );
}

status_t NodeMonitor::SetTo(const Directory& cDir, const PString& cPath, uint32_t nFlags, const Messenger& cTarget)
{
    if ( cDir.IsValid() == false ) {
	return( -1 );
    }
    int nNewMonitor = _CreateMonitor( cDir, cPath, nFlags, cTarget );
    if ( nNewMonitor < 0 ) {
	return( -1 );
    }
    if ( m_nMonitor >= 0 ) {
	delete_node_monitor( m_nMonitor );
    }
    m_nMonitor = nNewMonitor;
    return( 0 );
}

status_t NodeMonitor::SetTo(const Directory& cDir, const PString& cPath, uint32_t nFlags, const Handler* pcHandler, const Looper* pcLooper)
{
    return( SetTo( cDir, cPath, nFlags, Messenger( pcHandler, pcLooper ) ) );
}

status_t NodeMonitor::SetTo( const FileReference& cRef, uint32_t nFlags, const Messenger& cTarget )
{
    if ( cRef.IsValid() == false ) {
	return( -1 );
    }
    int nNewMonitor = _CreateMonitor( cRef, nFlags, cTarget );
    if ( nNewMonitor < 0 ) {
	return( -1 );
    }
    if ( m_nMonitor >= 0 ) {
	delete_node_monitor( m_nMonitor );
    }
    m_nMonitor = nNewMonitor;
    return( 0 );
}

status_t NodeMonitor::SetTo( const FileReference& cRef, uint32_t nFlags, const Handler* pcHandler, const Looper* pcLooper )
{
    return( SetTo( cRef, nFlags, Messenger( pcHandler, pcLooper ) ) );
}

status_t NodeMonitor::SetTo( const FSNode* pcNode, uint32_t nFlags, const Messenger& cTarget )
{
    if ( pcNode->IsValid() == false ) {
	return( -1 );
    }
    int nNewMonitor = _CreateMonitor( pcNode, nFlags, cTarget );
    if ( nNewMonitor < 0 ) {
	return( -1 );
    }
    if ( m_nMonitor >= 0 ) {
	delete_node_monitor( m_nMonitor );
    }
    m_nMonitor = nNewMonitor;
    return( 0 );
}

status_t NodeMonitor::SetTo( const FSNode* pcNode, uint32_t nFlags, const Handler* pcHandler, const Looper* pcLooper )
{
    return( SetTo( pcNode, nFlags, Messenger( pcHandler, pcLooper ) ) );
}

int NodeMonitor::GetMonitor() const
{
    return( m_nMonitor );
}
#endif