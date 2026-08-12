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

#ifndef SOCKETMGR_H_WIN32
#define SOCKETMGR_H_WIN32

#ifdef CONFIG_USE_SELECT

class Socket;

typedef std::set<BaseSocket*> SocketSet;

#define EVFILT_WRITE	0
#define EVFILT_READ		1

typedef enum E_SIGNAL_SOCKET_TYPE
{
    esstClient  = 0,
    esstServer  = 1,
    esstNum
} E_SST;

class SocketMgr : public Singleton<SocketMgr>
{
public:
	SocketMgr();
	~SocketMgr();

	/// add a new socket to the epoll set and to the fd mapping
	void AddSocket(BaseSocket * pSocket, bool listenSocket);

	/// remove a socket from epoll set/fd mapping
	void RemoveSocket(BaseSocket * pSocket);

	/// closes all sockets
	void CloseAll();

	/// spawns worker threads
	void SpawnWorkerThreads();

	/// This is called when a socket has data to write for the first time.
	void WantWrite(BaseSocket * pSocket);

	/// Thread proc
	void thread_run(ThreadContext *pContext);
    
    /// Before thread shudown
	void thread_OnShutdown(ThreadContext *pContext);

private:
	fd_set					m_writableSet;
	fd_set					m_readableSet;
    bool                    m_hasListenSocket;
    
    //for signaling
    SOCKET                  m_socketPair[esstNum];
    
    //holding sockets ptrs
	SocketSet               m_sockets;
	std::recursive_mutex	m_socketLock;
};

#define sSocketMgr SocketMgr::getSingleton()

class SocketWorkerThread : public ThreadContext
{
public:
    //ThreadContext
	bool run();
    void OnShutdown();
};

#endif
#endif