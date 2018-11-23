#include "stdafx.h"
#include "Client.h"
#include "Logger.h"
#include "process.h"
#include "Format.h"

CRITICAL_SECTION Client::g_cs;

//joseph-test
int sizeAddition = 0;

Client::Client():m_pBuffer(NULL)
{
    InitializeCriticalSection(&g_cs);
    isInitFIN = false;
    m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(m_socket == INVALID_SOCKET)
    {
		WRITE_ERROR_LOG("��ʼ���׽���ʧ��", WSAGetLastError());
		printf("init socket failed: %d\n", WSAGetLastError());
    }
    else
    {
		int waitTimeout = 30000;
		int nRes = 0;
		nRes = setsockopt(m_socket, SOL_SOCKET, SO_SNDTIMEO, (char*)&waitTimeout, sizeof(waitTimeout));
		if(nRes == SOCKET_ERROR)
		{
			WRITE_WARN_LOG("���÷��ͳ�ʱʱ��ʧ��", WSAGetLastError());
		}
		nRes = setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO, (char*)&waitTimeout, sizeof(waitTimeout));
		if(nRes == SOCKET_ERROR)
		{
			WRITE_WARN_LOG("���ý��ճ�ʱʱ��ʧ��", WSAGetLastError());
		}
        //struct linger so_linger;
        //so_linger.l_onoff = TRUE;
        //so_linger.l_linger = 10;
        //setsockopt(m_socket, SOL_SOCKET, SO_LINGER, (const char*)&so_linger, sizeof so_linger);
        WRITE_INFO_LOG("��ʼ���׽��ֳɹ�");
		isInitFIN = true;
    }
}

void Client::InitSettings(option_settings settings)
{
	ResetSettings();
	m_iport = settings.port;
	m_interval = settings.interval;
	m_bufSize = settings.bufsize;
	isUpload = settings.bUpload;
	m_signalUnit = settings.format;
	if(settings.time > 0 && settings.num == 0)
	{
		m_aimTime = settings.time;
		isTimeEnv = true;
	}
	else if(settings.time == 0 && settings.num > 0)
	{
		m_aimAmout = settings.num;
		isTimeEnv = false;
	}
}

int Client::Connect(LPCSTR ip, int iPort)
{
    if(!isInitFIN) 
	{
		WRITE_ERROR_LOG("δ��ʼ���׽���");
		return -1;
	}

	memset(&m_socketAddr, 0, sizeof(m_socketAddr));
    m_socketAddr.sin_family = AF_INET;
    m_socketAddr.sin_port = htons(iPort);

	if((m_socketAddr.sin_addr.s_addr = inet_addr(ip)) == INADDR_NONE)
	{
		PHOSTENT hp = NULL;
		if((hp = gethostbyname(ip)) != NULL)
		{
			memcpy(&(m_socketAddr.sin_addr), hp->h_addr, hp->h_length);
			m_socketAddr.sin_family = hp->h_addrtype;
		}
		else
		{
			WRITE_ERROR_LOG("��Ч������IP��ַ");
			printf("Cannot identify destination's address\n");
			return -1;
		}
	}

    int ret = connect(m_socket, (PSOCKADDR)&m_socketAddr, sizeof(m_socketAddr));
    if(ret == SOCKET_ERROR)
    {
		ret = WSAGetLastError();
		WRITE_ERROR_LOG("���ӷ�����ʧ��", ret);
		printf("Connect to %s failed: %d\n", inet_ntoa(m_socketAddr.sin_addr), ret);
        isInitFIN = false;
        return SOCKET_ERROR;
    }

    SOCKADDR_IN clientAddr;
    int iAddrSize = sizeof(clientAddr);
    getsockname(m_socket, (PSOCKADDR)&clientAddr, &iAddrSize);
	char ch[DEFALUT_INFOR_LEN] = {0};
	sprintf_s(ch, "���ӷ������ɹ����˿ں� %d", ntohs(clientAddr.sin_port));
	WRITE_INFO_LOG(ch);
	printf("------------------------------------------------------------\n");
	printf("Client connects with port %d successfully.\n", ntohs(clientAddr.sin_port));
	printf("------------------------------------------------------------\n");
	return ret;
}

int Client::PrepareSedOrRev()
{
	if(!isInitFIN) return -1;
	if(m_pBuffer != NULL)
	{
		delete m_pBuffer;
		m_pBuffer = NULL;
	}
	m_pBuffer = new char[m_bufSize];
	if(m_pBuffer == NULL)
	{
		WRITE_ERROR_LOG("�������ݻ������ռ�ʧ��");
		return -1;
	}
	srand((unsigned int)time(NULL));
	for(int i=0; i<m_bufSize; ++i)
	{
		m_pBuffer[i] = rand()%256;
	}
	client_hdr header = GenerateClientHdr();
	WRITE_INFO_LOG("������ָ�������");
	int res = send(m_socket, (const char*)&header, sizeof(header), 0);

	if(res != sizeof(header))
	{
		WRITE_WARN_LOG("������ָ����������ֲ��", res);
		return -1;
	}
	return 0;
}

void Client::StartSend()
{
	if(PrepareSedOrRev() == -1) return;
	//int rc;
	//int winLen = 0;
	//int len = sizeof(winLen);
	//rc = getsockopt(m_socket, SOL_SOCKET, SO_SNDBUF, (char*)&winLen, &len);
	//if(rc < 0)
	//	printf("getsockopt of sndbuf failed.\n");
	//else
	//	printf("sendbuf = %d\n", winLen);

	int res;
	WRITE_INFO_LOG("��ʼ��������");
	
	DWORD terminal;
	DWORD breakout = m_aimTime * 1000;
	DWORD start = GetCurMilliSec();
	//use for interval count
	if(m_interval > 0)
	{
		//Infor: �������������ʾ�߳�
		hIntervalEvent = CreateEvent(NULL, false, false, LPCTSTR("Local/SendInterval"));
		m_hIntervalEvent = (HANDLE)_beginthreadex(NULL, 0, &TS_Interval, this, 0, NULL);
		if(m_hIntervalEvent == INVALID_HANDLE_VALUE)
		{
			WRITE_ERROR_LOG("���������ʾ�����߳�ʧ��");
			SetEvent(hIntervalEvent);
		}
	}
	if(isTimeEnv)
	{//����ʱ�䳤�Ƚ��з���
		while(true)
		{
			res = send(m_socket, m_pBuffer, m_bufSize, 0);
			if(res == SOCKET_ERROR)
			{
				if(m_interval > 0 && m_hIntervalEvent != INVALID_HANDLE_VALUE)
				{
					SetEvent(hIntervalEvent);
					CloseHandle(m_hIntervalEvent);
				}
				WRITE_ERROR_LOG("��������ʧ��", WSAGetLastError());
				printf("Send failed :(\n");
				return;
			}

			EnterCriticalSection(&g_cs);
			m_stAmout += res;
			terminal = GetCurMilliSec();
			m_stTime = terminal - start;
			if(m_stTime >= breakout)
			{
				LeaveCriticalSection(&g_cs);
				break;
			}
			LeaveCriticalSection(&g_cs);
		}
	}
	else
	{//�������������з���
		int breakAmont = m_aimAmout;
		int sendSize = m_bufSize > m_aimAmout ? m_aimAmout : m_bufSize;
		while(true)
		{
			res = send(m_socket, m_pBuffer, sendSize, 0);
			if(res == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK)
			{
				if(m_interval > 0 && m_hIntervalEvent != INVALID_HANDLE_VALUE)
				{
					SetEvent(hIntervalEvent);
					CloseHandle(m_hIntervalEvent);
				}
				WRITE_ERROR_LOG("��������ʧ��", WSAGetLastError());
				printf("Send failed :(\n");
				return;
			}
			EnterCriticalSection(&g_cs);
			m_stAmout += res;
			breakAmont -= res;
			terminal = GetCurMilliSec();
			m_stTime = terminal - start;
			LeaveCriticalSection(&g_cs);
			if(breakAmont <= 0)
			{
				m_stTime = GetCurMilliSec() - start;
				break;
			}
			sendSize = m_bufSize > breakAmont ? breakAmont : m_bufSize;
		}
	}

	if(m_interval > 0 && m_hIntervalEvent != INVALID_HANDLE_VALUE)
	{
		Sleep(500);
		SetEvent(hIntervalEvent);
		CloseHandle(m_hIntervalEvent);
	}
	WRITE_INFO_LOG("����������ϣ���ʼ�������֪ͨ��Ϣ");
	client_tail tail;
	res = send(m_socket, (const char*)&tail, sizeof(tail), 0);
	if(res != sizeof(tail))
	{
		WRITE_ERROR_LOG("�������֪ͨ��Ϣʧ��", WSAGetLastError());
		return;
	}

	server_hdr srHeader;
	res = recv(m_socket, (char*)&srHeader, sizeof(server_hdr), 0);
	if(res == sizeof(server_hdr))
	{
		WRITE_INFO_LOG("���յ����������ص�ȷ�Ͻ�����Ϣ");
		if(srHeader.m_amount != m_stAmout)
		{
			WRITE_WARN_LOG("���������յ�������ͻ��˷��͵���������һ��");
			printf("Warn: amount of data sent are not in accordance with server recv\n");
		}
		///!!!
		//m_stTime = GetCurMilliSec();
	}
	else 
	{
		WRITE_ERROR_LOG("δ�ܽ��յ�����������ȷ����Ϣ", res);
		printf("Warn: failed to recv comfirm data from server\n");
	}
	///
	PrintResult();
	return;

	//eRevState = REV_NOT_HAPPEN;
	//HANDLE handleListern = (HANDLE)_beginthreadex(NULL, 0, &ListernFromServer, this, 0, NULL);
	//if(handleListern == INVALID_HANDLE_VALUE)
	//{
	//	WRITE_WARN_LOG("�������շ�����������Ϣ�߳�ʧ��");
	//}
	//else
	//{
	//	DWORD dwResult = WaitForSingleObject(handleListern, DEFAULT_WAIT_INTERVAL);
	//	if(eRevState == REV_NOT_HAPPEN)
	//	{
	//		WRITE_ERROR_LOG("�̵߳ȴ�ʱ�������δ�ܽ��յ�����������ȷ����Ϣ");
	//		printf("Warn: failed to recv comfirm data from server\n");
	//	}
	//	else
	//	{
	//		WRITE_INFO_LOG("���յ����������ص�ȷ�Ͻ�����Ϣ");
	//		if(eRevState == REV_YET_SUCCESS)
	//		{
	//			WRITE_WARN_LOG("���������յ�������ͻ��˷��͵���������һ��");
	//			printf("Warn: amount of data sent are not in accordance with server recv\n");
	//		}
	//	}
	//}

}

void Client::StartRev()
{
	if(PrepareSedOrRev() == -1) return;

	//int rc;
	//int winLen = 0;
	//int len = sizeof(winLen);
	//rc = getsockopt(m_socket, SOL_SOCKET, SO_RCVBUF, (char*)&winLen, &len);
	//if(rc < 0)
	//	printf("getsockopt of rcvbuf failed.\n");
	//else
	//	printf("rcvbuf = %d\n", winLen);

	int res;
	eRevState = REV_NOT_HAPPEN;
	//if(isTimeEnv)
	{
		server_hdr header;
		int sz_header = sizeof(header);
		WRITE_INFO_LOG("��ʼ��������");
		DWORD start = GetCurMilliSec();
		DWORD terminal;
		if(m_interval > 0)
		{
			//Infor: �������������ʾ�߳�
			hIntervalEvent = CreateEvent(NULL, false, false, LPCTSTR("Local/SendInterval"));
			m_hIntervalEvent = (HANDLE)_beginthreadex(NULL, 0, &TS_Interval, this, 0, NULL);
			if(m_hIntervalEvent == INVALID_HANDLE_VALUE)
			{
				WRITE_ERROR_LOG("�������������ʾ�߳�ʧ��");
				SetEvent(hIntervalEvent);
			}
		}
		while(true)
		{
			res = recv(m_socket, m_pBuffer, m_bufSize, 0);
			if(res == SOCKET_ERROR)
			{
				if(m_interval > 0 && m_hIntervalEvent != INVALID_HANDLE_VALUE)
				{
					SetEvent(hIntervalEvent);
					CloseHandle(m_hIntervalEvent);
				}
				if(WSAECONNRESET == WSAGetLastError())
				{
					WRITE_ERROR_LOG("�������Ͽ�����", WSAECONNRESET);
					printf("Server disconnects accidentally.\n");
				}
				else
				{
					WRITE_ERROR_LOG("��������ʧ��", WSAGetLastError());
					printf("recv failed:( %d\n", WSAGetLastError());
				}
				break;
			}
			else if(res == sz_header)
			{
				memcpy(&header, m_pBuffer, sz_header);
				if(header.flag != '!')
				{
					WRITE_WARN_LOG("�������ݴ�С��βָ����ͬ");
					EnterCriticalSection(&g_cs);
					m_stAmout += res;
					terminal = GetCurMilliSec();
					m_stTime = terminal - start;
					LeaveCriticalSection(&g_cs);
					continue;
				}
				if(m_interval > 0 && m_hIntervalEvent != INVALID_HANDLE_VALUE)
				{
					Sleep(500);
					SetEvent(hIntervalEvent);
					CloseHandle(m_hIntervalEvent);
				}
				WRITE_INFO_LOG("�����������");

				if(header.m_amount != m_stAmout)
				{
					eRevState = REV_YET_SUCCESS;
					break;
				}
				eRevState = REV_AND_SUCCESS; 
				break;
			}

			EnterCriticalSection(&g_cs);
			m_stAmout += res;
			terminal = GetCurMilliSec();
			m_stTime = terminal - start;
			LeaveCriticalSection(&g_cs);
		}
	}
	if(eRevState != REV_NOT_HAPPEN)
	{
		if(eRevState == REV_YET_SUCCESS)
		{
			WRITE_WARN_LOG("�ͻ��˽��յ���������������͵���������һ��");
			printf("Warn: amount of data recv are not in accordance with server sent\n");
		}
	}
	else
	{
		printf("Warn: Recv from Server imcompletely\n");
	}
	PrintResult();
	return;
}

unsigned int Client::TS_Interval(LPVOID lpParam)
{
    Client* pClient = (Client*)lpParam;
	int interval = pClient->m_interval * 1000;
	int curDoneSize;
	double curDoneSec;
	int lastDoneSize = 0;
	double lastDoneSec = 0;
	printf("            Interval      Transfer      Bandwidth\n");
	char buffer[64] = {0};
    while(WaitForSingleObject(pClient->hIntervalEvent, interval) != WAIT_OBJECT_0)
    {
		EnterCriticalSection(&g_cs);
		curDoneSize = pClient->m_stAmout;
		curDoneSec = pClient->m_stTime * 1.0 / 1000;
        double dSize = (double)(curDoneSize - lastDoneSize);
		double dInterval = (double)(curDoneSec - lastDoneSec);
		assert(dInterval != 0);
		if(dInterval < 0.005) continue;
		ByteSprintf(buffer, 32, dSize, toupper((int)pClient->m_signalUnit));
		ByteSprintf(buffer+32, 32, dSize / dInterval, pClient->m_signalUnit);
		printf(report_interval, lastDoneSec, curDoneSec, buffer, buffer+32);
		//printf("%.1lf MBytes %.1lf Mbits/sec\n", 
		//	dSize / 1024 / 1024, dSize / 1000 / 1000 * 8 / pClient->m_interval);
        lastDoneSize = curDoneSize;
		lastDoneSec = curDoneSec;
		LeaveCriticalSection(&g_cs);
    }
    return 0;
}

unsigned int WINAPI Client::ListernFromServer(LPVOID lpParam)
{
	//int res;
	//server_hdr s_header;
	//Client* pClient = (Client*)lpParam;
	//

	//res = recv(pClient->m_socket, (char*)&s_header, sizeof(s_header), 0);

	//if(res == sizeof(s_header))
	//{
	//	if(s_header.m_amount == pClient->m_stAmout)
	//	{
	//		pClient->eRevState = REV_AND_SUCCESS;
	//	}
	//	else
	//	{
	//		pClient->eRevState = REV_YET_SUCCESS;
	//	}
	//}
	return 0;
}

Client::~Client()
{
    DeleteCriticalSection(&g_cs);
    shutdown(m_socket, SD_BOTH);
    closesocket(m_socket);
}

client_hdr Client::GenerateClientHdr()
{
	client_hdr header;
	header.isUpload = isUpload;
	header.bufsize = m_bufSize;
	if(isTimeEnv)
		header.m_amount = m_aimTime;
	else
		header.m_amount = -m_aimAmout;
	return header;
}

void Client::PrintResult()
{
	char buffer[65] = {0};
	char bufferWrite[128] = {0};
	double dSentSize = (double)m_stAmout;
	double dSentTime = m_stTime > 0 ? (double)m_stTime : 1.0;
	double dTotalNum = dSentSize;
	double dTotalTime = dSentTime / 1000;
	ByteSprintf(buffer, 32, dTotalNum, toupper((int)m_signalUnit));
	ByteSprintf(buffer+32, 32, dTotalNum / dTotalTime, m_signalUnit);
	if(isTimeEnv)
	{
		if(isUpload)
		{
			printf(result_t_upload, (double)0, dTotalTime, buffer, buffer+32);
			sprintf_s(bufferWrite, 128, result_t_upload, (double)0, dTotalTime, buffer, buffer+32);
			bufferWrite[strlen(bufferWrite)-1] = '\0';
			WRITE_INFO_LOG(bufferWrite);
		}
		else
		{
			printf(result_t_download, (double)0, dTotalTime, buffer, buffer+32);
			sprintf_s(bufferWrite, 128, result_t_download, (double)0, dTotalTime, buffer, buffer+32);
			bufferWrite[strlen(bufferWrite)-1] = '\0';
			WRITE_INFO_LOG(bufferWrite);
		}
	}
	else
	{
		if(isUpload)
		{
			printf(result_n_upload, (double)0, dTotalTime, buffer, buffer+32);
			sprintf_s(bufferWrite, 128, result_n_upload, (double)0, dTotalTime, buffer, buffer+32);
			bufferWrite[strlen(bufferWrite)-1] = '\0';
			WRITE_INFO_LOG(bufferWrite);
		}
		else
		{
			printf(result_n_download, (double)0, dTotalTime, buffer, buffer+32);
			sprintf_s(bufferWrite, 128, result_n_download, (double)0, dTotalTime, buffer, buffer+32);
			bufferWrite[strlen(bufferWrite)-1] = '\0';
			WRITE_INFO_LOG(bufferWrite);
		}
	}
}


