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

#ifdef CONFIG_USE_IOCP

Socket::Socket(SOCKET fd, uint32 sendbuffersize, uint32 recvbuffersize) : m_fd(fd), m_connected(false),	m_deleted(false), m_readBuffer(recvbuffersize), m_writeBuffer(sendbuffersize)
{
	// IOCP Member Variables
	m_writeLock = 0;
	m_completionPort = 0;

	// Check for needed fd allocation.
	if(m_fd == 0)
	{
		m_fd = SocketOps::CreateTCPFileDescriptor();
	}
}

Socket::~Socket()
{

}

bool BaseSocket::Connect(SOCKET fd, const sockaddr_in *peer, uint32 timeout)
{
	//set non-blocking
	SocketOps::Nonblocking(fd);

	//try to connect
	int result = connect(fd, (const sockaddr*)peer, sizeof(sockaddr_in));
	if (result == 0 || WSAGetLastError() != WSAEWOULDBLOCK)
	{
		Log.Error(__FUNCTION__, "result == %d, errno = %d", result, WSAGetLastError());
		std::this_thread::sleep_for(std::chrono::milliseconds(timeout * 1000));
		return false;
	}

	//async socket connect
	int count;
	fd_set fdset;
	struct timeval tv;
	bool oResult = false;

	FD_ZERO(&fdset);
	FD_SET(fd, &fdset);
	tv.tv_sec = timeout;
	tv.tv_usec = 0;

	count = select(FD_SETSIZE, NULL, &fdset, NULL, &tv);
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

void Socket::Accept(const sockaddr_in * peer)
{
	memcpy(&m_peer, peer, sizeof(sockaddr_in));
	_OnConnect();
}

void Socket::_OnConnect()
{
	// set common parameters on the file descriptor
	SocketOps::Nonblocking(m_fd);
	SocketOps::DisableBuffering(m_fd);
	m_connected = true;

	// IOCP stuff
	AssignToCompletionPort();
	SetupReadEvent();

	sSocketMgr.AddSocket(this);

	// Call virtual onconnect
	OnConnect();
}

bool Socket::BurstSend(const void * data, size_t bytes)
{
	return m_writeBuffer.Write(data, bytes);
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
	if(ip != NULL)
		return std::string(ip);
	else
		return std::string("noip");
}

void Socket::Disconnect()
{
	m_connected = false;

	// remove from mgr
	sSocketMgr.RemoveSocket(this);

	SocketOps::CloseSocket(m_fd);

	// Call virtual ondisconnect
	OnDisconnect();

	if(!m_deleted)
	{
		Delete();
	}
}

void Socket::Delete()
{
	if(m_deleted)
		return;

	m_deleted = true;

	if(m_connected) 
	{
		Disconnect();
	}

	sSocketGarbageCollector.QueueSocket(this);
}

void Socket::WriteCallback(size_t len)
{
	if(m_writeBuffer.GetContiguiousBytes())
	{
		DWORD w_length = 0;
		DWORD flags = 0;

		// attempt to push all the data out in a non-blocking fashion.
		WSABUF buf;
		buf.len = (u_long)m_writeBuffer.GetContiguiousBytes();
		buf.buf = (char*)m_writeBuffer.GetBufferStart();

		m_writeEvent.Mark();
		m_writeEvent.Reset(SOCKET_IO_EVENT_WRITE_END);

		int r = WSASend(m_fd, &buf, 1, &w_length, flags, &m_writeEvent.m_overlap, NULL);
		if(r == SOCKET_ERROR)
		{
			int errNumber = WSAGetLastError();
			if(errNumber != WSA_IO_PENDING)
			{
				m_writeEvent.Unmark();
				DecSendLock();
				Disconnect();
			}
		}
	}
	else
	{
		// Write operation is completed.
		DecSendLock();
	}
}

void Socket::SetupReadEvent()
{
	DWORD r_length = 0;
	DWORD flags = 0;
	WSABUF buf;
	buf.len = (u_long)m_readBuffer.GetSpace();
	buf.buf = (char*)m_readBuffer.GetBuffer();	

	m_readEvent.Mark();
	m_readEvent.Reset(SOCKET_IO_EVENT_READ_COMPLETE);

	int r = WSARecv(m_fd, &buf, 1, &r_length, &flags, &m_readEvent.m_overlap, NULL);
	if(r == SOCKET_ERROR)
	{
		int errNumber = WSAGetLastError();
		if(errNumber != WSA_IO_PENDING)
		{
			m_readEvent.Unmark();
			Disconnect();
		}
	}
}

void Socket::ReadCallback(size_t len)
{
	m_readBuffer.IncrementWritten(len);
	OnRead();
	SetupReadEvent();
}

void Socket::AssignToCompletionPort()
{
	//add to existing completionPort
	CreateIoCompletionPort((HANDLE)m_fd, m_completionPort, (ULONG_PTR)this, 0);
}

void Socket::BurstPush()
{
	if(AcquireSendLock())
	{
		WriteCallback(0);
	}
}

void Socket::PostEvent(int events)
{
	//Not for win32
}

#endif