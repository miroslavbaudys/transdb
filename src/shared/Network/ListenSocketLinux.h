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

#ifndef _LISTENSOCKET_H
#define _LISTENSOCKET_H

template<class T>
class ListenSocket : public BaseSocket
{
public:
	explicit ListenSocket(const char * hostname, u_short port) : m_fd(SocketOps::CreateTCPFileDescriptor()), m_deleted(false), m_connected(false)
	{
		if(m_fd < 0)
		{
		 	Log.Error(__FUNCTION__, "ListenSocket constructor: could not create socket() %u (%s)", errno, strerror(errno));
			throw std::runtime_error("could not create socket()");
		}
        
		//socket settings
		SocketOps::ReuseAddr(m_fd);
		SocketOps::Blocking(m_fd);
		SocketOps::SetTimeout(m_fd, 60);
        
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
			hostent * h = gethostbyname(hostname);
			if(!h)
			{
				Log.Error(__FUNCTION__, "Could not resolve listen address");
				throw std::runtime_error("Could not resolve listen address");
			}
			memcpy(&address.sin_addr, h->h_addr_list[0], sizeof(in_addr));
		}

		address.sin_family = AF_INET;
		address.sin_port = ntohs(port);
        
        //bind
		if(::bind(m_fd, (const sockaddr*)&address, sizeof(sockaddr_in)) < 0)
		{
			Log.Error(__FUNCTION__, "Bind unsuccessful on port %u.", port);
			throw std::runtime_error("Could not bind");
		}

        //listen
		if(::listen(m_fd, SOMAXCONN) < 0)
		{
			Log.Error(__FUNCTION__, "Unable to listen on port %u.", port);
			throw std::runtime_error("Could not listen");
		}
        
        //set variables
		m_connected = true;
		m_deleted	= false;
		
		// add to mgr
		sSocketMgr.AddSocket(this, true);
	}

	~ListenSocket()
	{
		Close();
	}

	void ReadCallback(size_t len)
	{
		if(!m_connected)
			return;
        
        //variables
        SOCKET newFd;
        struct sockaddr_in newPeer;
        socklen_t newPeerLen = sizeof(sockaddr_in);
        
        //accept new socket
		newFd = ::accept(m_fd, (sockaddr*)&newPeer, &newPeerLen);
		if(newFd > 0)
		{
			T * s = new T(newFd);
			s->Accept(&newPeer);
		}
	}

	void OnError(int errcode) {}
	void OnRead() {}
	void OnConnect() {}
	void OnDisconnect() {}
	void Accept(const sockaddr_in * peer) {}
	void WriteCallback(size_t len) {}
    void BurstBegin() {}
    void BurstEnd() {}
    bool BurstSend(const void * data, size_t bytes)		{ return false; }
    void BurstPush() {}
    void PostEvent(int events) {}
	void IncSendLock() {}
	void DecSendLock() {}
	bool AcquireSendLock()								{ return false; }
	bool IsOpen() 										{ return m_connected; }	
	bool Writable() const                               { return false; }
	void Delete()										{ Close(); }
	void Disconnect()									{ Close(); }
    
	void Close()
	{
		// prevent a race condition here.
		bool mo = m_connected;
		m_connected = false;
		m_deleted = true;

		//remove from socketmgr
		sSocketMgr.RemoveSocket(this);

		if(mo)
		{
			shutdown(m_fd, SD_BOTH);
			SocketOps::CloseSocket(m_fd);
		}
	}
    
    /** Are we connected?
     */
	bool IsConnected() const    { return m_connected; }
	bool IsDeleted() const      { return m_deleted; }
    
	/** Returns the socket's file descriptor
	 */
	SOCKET GetFd() const        { return m_fd; }
    
private:
	/** This socket's file descriptor
	 */
	SOCKET              m_fd;
    
	/** deleted/disconnected markers
	 */
    std::atomic<bool>	m_deleted;
	std::atomic<bool>   m_connected;
};

#endif

