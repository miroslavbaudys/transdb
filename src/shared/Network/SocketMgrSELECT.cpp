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
#include "../clib/Network/socketpair.h"

#ifdef CONFIG_USE_SELECT

initialiseSingleton(SocketMgr);

/** Unblocks select from wait
 */
INLINE static void SignalSelect(SOCKET fd)
{
    uint8 flag = 0;
    if(send(fd, (const char*)&flag, sizeof(flag), 0) < 0)
    {
        Log.Error(__FUNCTION__, "Client signal socket broken.");
    }
}

SocketMgr::SocketMgr() : m_hasListenSocket(false)
{
#ifdef WIN32
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2,2), &wsaData);
#endif

	FD_ZERO(&m_readableSet);
	FD_ZERO(&m_writableSet);
    
    //create connected socket pair
    if(dumb_socketpair(m_socketPair, 0) != 0)
    {
        Log.Error(__FUNCTION__, "Cannot create socket pair.");
        exit(EXIT_FAILURE);
    }
    
    //add server socket to readable set
    FD_SET(m_socketPair[esstServer], &m_readableSet);
}

SocketMgr::~SocketMgr()
{
    //close signal sockets
    SocketOps::CloseSocket(m_socketPair[esstServer]);
    SocketOps::CloseSocket(m_socketPair[esstClient]);
    
#ifdef WIN32
	WSACleanup();
#endif
}

void SocketMgr::SpawnWorkerThreads()
{
	ThreadPool.ExecuteTask(new SocketWorkerThread());
}

void SocketMgr::AddSocket(BaseSocket* pSocket, bool listenSocket)
{
	std::lock_guard<std::recursive_mutex> rGuard(m_socketLock);
	if (m_sockets.find(pSocket) == m_sockets.end())
	{
		if(pSocket->Writable())
			FD_SET(pSocket->GetFd(), &m_writableSet);
		else
			FD_SET(pSocket->GetFd(), &m_readableSet);
		m_sockets.insert(pSocket);
        
        //dirty hack for listen socket
        if(listenSocket)
        {
            m_hasListenSocket = true;
        }
        
        //signal
        SignalSelect(m_socketPair[esstClient]);
	}
}

void SocketMgr::RemoveSocket(BaseSocket * pSocket)
{
	std::lock_guard<std::recursive_mutex> rGuard(m_socketLock);
	if (m_sockets.find(pSocket) != m_sockets.end())
	{
        //remove from read and write send
		FD_CLR(pSocket->GetFd(), &m_readableSet);
		FD_CLR(pSocket->GetFd(), &m_writableSet);
		m_sockets.erase(pSocket);
        
        //signal
        SignalSelect(m_socketPair[esstClient]);
	}
}

void SocketMgr::WantWrite(BaseSocket * pSocket)
{
    //we are allready locked
	FD_SET(pSocket->GetFd(), &m_writableSet);
    
    //signal
    SignalSelect(m_socketPair[esstClient]);
}

void SocketMgr::thread_run(ThreadContext *pContext)
{
	fd_set readable;
	fd_set writable;
    fd_set except;
    BaseSocket* pSocket;
    SocketSet::iterator itr, itr2;

	while(pContext->GetThreadRunning())
	{
		/* copy the sets */
		memcpy(&readable, &m_readableSet, sizeof(m_readableSet));
		memcpy(&writable, &m_writableSet, sizeof(m_writableSet));
        
        /** clear the exception set for the next loop
         */
        FD_ZERO(&except);
        
        /** clear the writable set for the next loop
         *  will work like EV_ONESHOT
         */
        if(m_hasListenSocket == false)
        {
            FD_ZERO(&m_writableSet);
        }
        
		//poll sockets status
		int fd_count = select(FD_SETSIZE, &readable, &writable, &except, NULL);
		if(fd_count < 0)
		{
#ifdef WIN32
			Log.Error(__FUNCTION__, "select fd_count: %d errno: %d", fd_count, WSAGetLastError());
#else
   			Log.Error(__FUNCTION__, "select fd_count: %d errno: %d", fd_count, errno);
#endif
		}
		else if(fd_count > 0)
		{
            //check signal socket
            if(FD_ISSET(m_socketPair[esstServer], &readable))
            {
                uint8 flag;
                if(recv(m_socketPair[esstServer], (char*)&flag, sizeof(flag), 0) <= 0)
                {
                    Log.Error(__FUNCTION__, "Server signal socket broken");
                }
            }
            
            //lock socket set
            std::lock_guard<std::recursive_mutex> rGuard(m_socketLock);
            
            //check changes on sockets
			for(itr = m_sockets.begin(); itr != m_sockets.end();)
			{
				itr2 = itr;
				++itr;
				pSocket = (*itr2);

				// If the ReadSet is marked for this socket then this means data
				// is available to be read on the socket
				if(FD_ISSET(pSocket->GetFd(), &readable))
				{
					if (pSocket->IsConnected())
					{
						pSocket->ReadCallback(0);
					}
				}
                
				// If the WriteSet is marked on this socket then this means the internal
				// data buffers are available for more data
				if(FD_ISSET(pSocket->GetFd(), &writable))
				{
					if(pSocket->IsConnected())
					{
						pSocket->BurstBegin();                      // Lock receive mutex
						pSocket->WriteCallback(0);                  // Perform actual send()
                        if(pSocket->IsConnected())
                        {
                            if(pSocket->Writable())
                            {
                                pSocket->PostEvent(EVFILT_WRITE);   // Still remaining data.
                            }
                            else
                            {
                                pSocket->DecSendLock();
                            }
                        }
						pSocket->BurstEnd();						// Unlock
					}
				}
                
                //Exception set -> socket error disconnect and make cleanup
                if(FD_ISSET(pSocket->GetFd(), &except))
                {
#ifdef WIN32
                    pSocket->OnError(WSAGetLastError());
#else
                    pSocket->OnError(errno);
#endif
                }
			}
		}
	}
}

void SocketMgr::thread_OnShutdown(ThreadContext *pContext)
{
    //signal and wake up thread for shutdown
    SignalSelect(m_socketPair[esstClient]);
}

bool SocketWorkerThread::run()
{
	CommonFunctions::SetThreadName("SocketWorker thread");
	sSocketMgr.thread_run(this);
	return true;
}

void SocketWorkerThread::OnShutdown()
{
    ThreadContext::OnShutdown();
    sSocketMgr.thread_OnShutdown(this);
}

void SocketMgr::CloseAll()
{
	LockingPtr<SocketSet> pSockets(m_sockets, NULL);
	std::list<BaseSocket*> tokill;

	m_socketLock.lock();
	for (SocketSet::iterator itr = pSockets->begin(); itr != pSockets->end(); ++itr)
	{
		tokill.push_back(*itr);
	}
	m_socketLock.unlock();
	
	for (std::list<BaseSocket*>::iterator itr = tokill.begin(); itr != tokill.end(); ++itr)
	{
		(*itr)->Disconnect();
	}

	size_t size = 0;
	do
	{
		m_socketLock.lock();
		size = pSockets->size();
		m_socketLock.unlock();
	}while(size);
}

#endif