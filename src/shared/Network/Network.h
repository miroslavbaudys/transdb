/*
 * Game server
 * Copyright (C) 2010 Miroslav 'Wayland' Kudrnac
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */


#ifndef NETWORK_H
#define NETWORK_H

#include "../Logs/Log.h"
#include "CircularBuffer.h"
#include "SocketDefines.h"
#include "SocketOps.h"

#ifdef CONFIG_USE_IOCP
	#include "BaseSocket.h"
	#include "SocketIOCP.h"
	#include "SocketMgrIOCP.h"
	#include "ListenSocketWin32.h"
#endif

#ifdef CONFIG_USE_SELECT
    #include "BaseSocket.h"
    #include "SocketSELECT.h"
    #include "SocketMgrSELECT.h"
    #ifdef WIN32
        #include "ListenSocketWin32.h"
    #else
        #include "ListenSocketLinux.h"
    #endif
#endif

#ifdef CONFIG_USE_EPOLL
	#include "BaseSocket.h"
	#include "SocketEPOLL.h"
	#include "SocketMgrEPOLL.h"
	#include "ListenSocketLinux.h"
#endif

#ifdef CONFIG_USE_KEVENT
    #include "BaseSocket.h"
    #include "SocketKEVENT.h"
    #include "SocketMgrKEVENT.h"
    #include "ListenSocketLinux.h"
#endif

#include "SocketGarbageCollector.h"

/** Connect to a server.
* @param hostname Hostname or IP address to connect to
* @param port Port to connect to
* @param timeout connect timeout in seconds
* @return templated type if successful, otherwise null
*/
template<class T>
static T* ConnectTCPSocket(const char * hostname, u_short port, uint32 timeout = 3)
{
	struct hostent * ci = gethostbyname(hostname);
	if(ci == NULL)
		return NULL;
    
    //create peer info
    sockaddr_in peer;
	peer.sin_family = ci->h_addrtype;
	peer.sin_port = ntohs(port);
	memcpy(&peer.sin_addr.s_addr, ci->h_addr_list[0], ci->h_length);
    
    //create socket fd
    SOCKET fd = SocketOps::CreateTCPFileDescriptor();
    
    //try to connect
    bool status = T::Connect(fd, &peer, timeout);
    if(status)
    {
        T * s = new T(fd);
        s->Accept(&peer);
        return s;
    }
    else
    {
        SocketOps::CloseSocket(fd);
        return NULL;
    }
}

#endif
