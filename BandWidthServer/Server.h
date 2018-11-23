#pragma once

#include "stdafx.h"
#include "InnerClient.h"

class Server
{
    public:
        Server(int iPort = DEFAULT_PORT);
        int Create();
        int Accept();
        virtual ~Server();
        SOCKET Get() { return m_socket; }
        void Set(SOCKET val) { m_socket = val; }

        static DWORD WINAPI ListenThread(LPVOID lpParam);
        static unsigned int WINAPI ClientThread(LPVOID lpParam);

		//////////////////////////////////////////////////////////////////////////
		int Run();

    protected:
    private:
        SOCKET m_socket;
        char *m_pBuffer;
        UINT  m_bufSize;
        int m_iPort;
        SOCKADDR_IN m_socketAddr;

        HANDLE m_threadArry[MAX_DEFAULT_THREAD_NUM];
        int m_curThreadNum;

		//////////////////////////////////////////////////////////////////////////
		 vector<InnerClient> clientInfor;

		 DWORD GetCurMilliSec()
		 {
			 return (DWORD)clock();
		 }
};















