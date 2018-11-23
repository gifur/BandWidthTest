#pragma  once

#include "stdafx.h"

const int DEFAULT_WAIT_INTERVAL = 30000;

enum RevState
{
	REV_YET_SUCCESS = 0,
	REV_AND_SUCCESS,
	REV_NOT_HAPPEN
};

class Client
{
    public:
        Client();
        ~Client();

        SOCKET Getsocket() { return m_socket; }
        void Setsocket(SOCKET val) { m_socket = val; }

        int Connect(LPCSTR ip, int iPort);
		int PrepareSedOrRev();
        void StartSend();
		void StartRev();

        static unsigned int WINAPI TS_Interval(LPVOID lpParam);
		static unsigned int WINAPI ListernFromServer(LPVOID lpParam);
        static CRITICAL_SECTION g_cs;

		void InitSettings(option_settings settings);
		client_hdr GenerateClientHdr();
		int m_interval;

		RevState eRevState;
		void PrintResult();

    protected:
        HANDLE m_hSendThread;
        HANDLE m_hRevThread;
        HANDLE hIntervalEvent;
        HANDLE m_hIntervalEvent;
    private:
        SOCKET m_socket;
        int m_iport;
		SOCKADDR_IN m_socketAddr;
        char *m_pBuffer;
        int  m_bufSize;

        bool isInitFIN;
		bool isTimeEnv;
		bool isUpload;

        DWORD m_dwCount;

        int m_stAmout;
        int m_aimAmout;
        DWORD m_aimTime;
        DWORD m_stTime;
		char m_signalUnit;

    private:
        void ResetSettings()
		{
			isTimeEnv = true;
			m_aimTime = DEFAULT_SEND_TIME;
			m_aimAmout = DEFAULT_SEND_NUM;
			m_stTime = 0;
			m_stAmout = 0;
		}

		DWORD GetCurMilliSec()
		{
			return (DWORD)clock();
		}
};

