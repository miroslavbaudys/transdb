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

#include "Network.h"

#ifdef CONFIG_USE_SELECT

//for send and recv
#ifdef WIN32
	typedef int ssize_t;
#endif

Socket::Socket(SOCKET fd, size_t readbuffersize, size_t writebuffersize) : m_readBuffer(readbuffersize), m_writeBuffer(writebuffersize)
{
	//set fd
	m_fd = fd;

	// Check for needed fd allocation.
	if (m_fd == 0)
	{
		m_fd = SocketOps::CreateTCPFileDescriptor();
	}

	/* switch the socket to nonblocking mode */
	SocketOps::Nonblocking(m_fd);

	/* disable nagle buffering by default */
	SocketOps::DisableBuffering(m_fd);

	/* set keep alive */
	SocketOps::KeepAlive(m_fd);
    
    /* set send recv timeout */
    SocketOps::SetTimeout(m_fd, SOCKET_SEND_RECV_TIMEOUT);

	m_writeLock     = 0;
	m_deleted       = false;
	m_connected     = false;
}

bool BaseSocket::Connect(SOCKET fd, const sockaddr_in *peer, uint32 timeout)
{
	//set non-blocking
	SocketOps::Nonblocking(fd);

	//try to connect
	int result = connect(fd, (const sockaddr*)peer, sizeof(sockaddr_in));
#ifdef WIN32
	if (result == 0 || WSAGetLastError() != WSAEWOULDBLOCK)
	{
		Log.Error(__FUNCTION__, "result == %d, errno = %d", result, WSAGetLastError());
		std::this_thread::sleep_for(std::chrono::milliseconds(timeout * 1000));
		return false;
	}
#else
    if(result == 0 || errno != EINPROGRESS)
    {
        Log.Error(__FUNCTION__, "result == %d, errno = %d", result, errno);
        std::this_thread::sleep_for(std::chrono::milliseconds(timeout * 1000));
        return false;
    }
#endif

	//async socket connect
	int count;
	fd_set fdset;
	struct timeval tv;
	bool oResult = false;

	FD_ZERO(&fdset);
	FD_SET(fd, &fdset);
	tv.tv_sec = timeout;
	tv.tv_usec = 0;

    count = select(fd + 1, NULL, &fdset, NULL, &tv);
	if (count == 1)
	{
		int so_error;
		socklen_t len = sizeof(so_error);
		getsockopt(fd, SOL_SOCKET, SO_ERROR, (char*)&so_error, &len);
		if (so_error == 0)
		{
			oResult = true;
		}
		else
		{
			Log.Error(__FUNCTION__, "SO_ERROR: %d", so_error);
		}
	}
	return oResult;
}

Socket::~Socket()
{

}

void Socket::Accept(const sockaddr_in * peer)
{
	memcpy(&m_peer, peer, sizeof(sockaddr_in));
	_OnConnect();
}

void Socket::_OnConnect()
{
	m_connected = true;

	//add to socket mgr
	sSocketMgr.AddSocket(this, false);

	// Call virtual onconnect
	OnConnect();
}

/* This is called when the socket engine gets an event on the socket */
void Socket::ReadCallback(size_t len)
{
	/* Any other platform, we have to call recv() to actually get the data. */
	size_t space = m_readBuffer.GetSpace();
	ssize_t bytes = recv(m_fd, (char*)m_readBuffer.GetBuffer(), space, 0);
	if (bytes <= 0)
	{
		Disconnect();
	}
	else
	{
		m_readBuffer.IncrementWritten(bytes);
		OnRead();
	}
}

/* This is called when the socket engine gets an event on the socket */
void Socket::WriteCallback(size_t len)
{
	// We should already be locked at this point, so try to push everything out.
	ssize_t bytes = send(m_fd, (char*)m_writeBuffer.GetBufferStart(), m_writeBuffer.GetContiguiousBytes(), 0);
	if (bytes < 0)
	{
		Disconnect();
		return;
	}

	m_writeBuffer.Remove(bytes);
}

bool Socket::BurstSend(const void * data, size_t bytes)
{
	return m_writeBuffer.Write(data, bytes);
}

void Socket::BurstPush()
{
	if (AcquireSendLock())
	{
		PostEvent(EVFILT_WRITE);
	}
}

void Socket::Disconnect()
{
	if (!m_connected)
		return;

	m_connected = false;

	OnDisconnect();
	sSocketMgr.RemoveSocket(this);
	SocketOps::CloseSocket(m_fd);

	if (!m_deleted)
	{
		Delete();
	}
}

void Socket::Delete()
{
	if (m_deleted)
		return;

	m_deleted = true;

	if (m_connected)
	{
		Disconnect();
	}

	sSocketGarbageCollector.QueueSocket(this);
}

void Socket::OnError(int errcode)
{
	Log.Debug(__FUNCTION__, "Error number: %u", errcode);
	Disconnect();
}

bool Socket::Writable() const
{
	return (m_writeBuffer.GetSize() > 0) ? true : false;
}

std::string Socket::GetRemoteIP()
{
	char* ip = (char*)inet_ntoa(m_peer.sin_addr);
	if (ip != NULL)
		return std::string(ip);
	else
		return std::string("noip");
}

void Socket::PostEvent(int events)
{
	if (events == EVFILT_WRITE)
	{
		sSocketMgr.WantWrite(this);
	}
}


#endif