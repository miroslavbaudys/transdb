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


#ifndef LISTEN_SOCKET_WIN32_H
#define LISTEN_SOCKET_WIN32_H

#ifdef CONFIG_USE_IOCP

#include "../Threading/ThreadPool.h"

template<class T>
class ListenSocket : public ThreadContext
{
public:
	explicit ListenSocket(const char * hostname, u_short port) : m_socket(SocketOps::CreateTCPFileDescriptor()), m_opened(false), m_cp(sSocketMgr.GetCompletionPort())
	{
        if(m_socket == INVALID_SOCKET)
        {
		 	Log.Error(__FUNCTION__, "ListenSocket constructor: could not create socket %u", WSAGetLastError());
			throw std::runtime_error("could not create socket");
        }

		//socket settings
		SocketOps::ReuseAddr(m_socket);
		SocketOps::Blocking(m_socket);
		SocketOps::SetTimeout(m_socket, 60);

		//create sock address struct
		struct sockaddr_in address;
		memset(&address, 0, sizeof(sockaddr_in));

        //DNS -> IP
		if(!strcmp(hostname, "0.0.0.0"))
		{
			address.sin_addr.s_addr = htonl(INADDR_ANY);
		}
		else
		{
			struct hostent * h = gethostbyname(hostname);
			if(!h)
			{
				Log.Error(__FUNCTION__, "Could not resolve listen address");
				throw std::runtime_error("Could not resolve listen address");
			}
			memcpy(&address.sin_addr, h->h_addr_list[0], sizeof(in_addr));
		}

		address.sin_family = AF_INET;
		address.sin_port = ntohs(port);

		// bind.. well attempt to.
		int ret = ::bind(m_socket, (const sockaddr*)&address, sizeof(address));
		if(ret != 0)
		{
			Log.Error(__FUNCTION__, "Bind unsuccessful on port %u.", port);
			throw std::runtime_error("Could not bind");
		}

		ret = ::listen(m_socket, SOMAXCONN);
		if(ret != 0) 
		{
			Log.Error(__FUNCTION__, "Unable to listen on port %u.", port);
			throw std::runtime_error("Could not listen");
		}

        //set variables
		m_opened = true;
	}

	~ListenSocket()
	{
		Close();	
	}

	bool run()
	{
        T *pSocket;
        SOCKET aSocket;
		struct sockaddr_in newPeer;
		INT newPeerLen = sizeof(sockaddr_in);
        
		while(m_opened)
		{
			aSocket = WSAAccept(m_socket, (sockaddr*)&newPeer, (LPINT)&newPeerLen, NULL, NULL);
			if(aSocket == INVALID_SOCKET)
				continue;

			pSocket = new T(aSocket);
			pSocket->SetCompletionPort(m_cp);
			pSocket->Accept(&newPeer);
		}
		return true;
	}

	void Close()
	{
		// prevent a race condition here.
		bool mo = m_opened;
		m_opened = false;

		if(mo)
		{
			SocketOps::CloseSocket(m_socket);
		}
	}

	bool IsOpen() 
	{ 
		return m_opened; 
	}

private:
	SOCKET				m_socket;
    std::atomic<bool>   m_opened;
	HANDLE				m_cp;
};

#endif
#endif
