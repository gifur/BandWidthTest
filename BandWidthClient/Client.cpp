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
		WRITE_ERROR_LOG("初始化套接字失败", WSAGetLastError());
		printf("init socket failed: %d\n", WSAGetLastError());
    }
    else
    {
		int waitTimeout = 30000;
		int nRes = 0;
		nRes = setsockopt(m_socket, SOL_SOCKET, SO_SNDTIMEO, (char*)&waitTimeout, sizeof(waitTimeout));
		if(nRes == SOCKET_ERROR)
		{
			WRITE_WARN_LOG("设置发送超时时间失败", WSAGetLastError());
		}
		nRes = setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO, (char*)&waitTimeout, sizeof(waitTimeout));
		if(nRes == SOCKET_ERROR)
		{
			WRITE_WARN_LOG("设置接收超时时间失败", WSAGetLastError());
		}
        //struct linger so_linger;
        //so_linger.l_onoff = TRUE;
        //so_linger.l_linger = 10;
        //setsockopt(m_socket, SOL_SOCKET, SO_LINGER, (const char*)&so_linger, sizeof so_linger);
        WRITE_INFO_LOG("初始化套接字成功");
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
		WRITE_ERROR_LOG("未初始化套接字");
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
			WRITE_ERROR_LOG("无效服务器IP地址");
			printf("Cannot identify destination's address\n");
			return -1;
		}
	}

    int ret = connect(m_socket, (PSOCKADDR)&m_socketAddr, sizeof(m_socketAddr));
    if(ret == SOCKET_ERROR)
    {
		ret = WSAGetLastError();
		WRITE_ERROR_LOG("连接服务器失败", ret);
		printf("Connect to %s failed: %d\n", inet_ntoa(m_socketAddr.sin_addr), ret);
        isInitFIN = false;
        return SOCKET_ERROR;
    }

    SOCKADDR_IN clientAddr;
    int iAddrSize = sizeof(clientAddr);
    getsockname(m_socket, (PSOCKADDR)&clientAddr, &iAddrSize);
	char ch[DEFALUT_INFOR_LEN] = {0};
	sprintf_s(ch, "连接服务器成功，端口号 %d", ntohs(clientAddr.sin_port));
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
		WRITE_ERROR_LOG("分配数据缓冲区空间失败");
		return -1;
	}
	srand((unsigned int)time(NULL));
	for(int i=0; i<m_bufSize; ++i)
	{
		m_pBuffer[i] = rand()%256;
	}
	client_hdr header = GenerateClientHdr();
	WRITE_INFO_LOG("发送首指令到服务器");
	int res = send(m_socket, (const char*)&header, sizeof(header), 0);

	if(res != sizeof(header))
	{
		WRITE_WARN_LOG("发送首指令到服务器出现差错", res);
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
	WRITE_INFO_LOG("开始发送数据");
	
	DWORD terminal;
	DWORD breakout = m_aimTime * 1000;
	DWORD start = GetCurMilliSec();
	//use for interval count
	if(m_interval > 0)
	{
		//Infor: 开启间隔数据显示线程
		hIntervalEvent = CreateEvent(NULL, false, false, LPCTSTR("Local/SendInterval"));
		m_hIntervalEvent = (HANDLE)_beginthreadex(NULL, 0, &TS_Interval, this, 0, NULL);
		if(m_hIntervalEvent == INVALID_HANDLE_VALUE)
		{
			WRITE_ERROR_LOG("创建间隔显示数据线程失败");
			SetEvent(hIntervalEvent);
		}
	}
	if(isTimeEnv)
	{//根据时间长度进行发送
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
				WRITE_ERROR_LOG("发送数据失败", WSAGetLastError());
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
	{//根据数据量进行发送
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
				WRITE_ERROR_LOG("发送数据失败", WSAGetLastError());
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
	WRITE_INFO_LOG("发送数据完毕，开始发送完成通知消息");
	client_tail tail;
	res = send(m_socket, (const char*)&tail, sizeof(tail), 0);
	if(res != sizeof(tail))
	{
		WRITE_ERROR_LOG("发送完成通知消息失败", WSAGetLastError());
		return;
	}

	server_hdr srHeader;
	res = recv(m_socket, (char*)&srHeader, sizeof(server_hdr), 0);
	if(res == sizeof(server_hdr))
	{
		WRITE_INFO_LOG("接收到服务器返回的确认接收消息");
		if(srHeader.m_amount != m_stAmout)
		{
			WRITE_WARN_LOG("服务器接收到数据与客户端发送的数据量不一致");
			printf("Warn: amount of data sent are not in accordance with server recv\n");
		}
		///!!!
		//m_stTime = GetCurMilliSec();
	}
	else 
	{
		WRITE_ERROR_LOG("未能接收到服务器返回确认消息", res);
		printf("Warn: failed to recv comfirm data from server\n");
	}
	///
	PrintResult();
	return;

	//eRevState = REV_NOT_HAPPEN;
	//HANDLE handleListern = (HANDLE)_beginthreadex(NULL, 0, &ListernFromServer, this, 0, NULL);
	//if(handleListern == INVALID_HANDLE_VALUE)
	//{
	//	WRITE_WARN_LOG("创建接收服务器返回消息线程失败");
	//}
	//else
	//{
	//	DWORD dwResult = WaitForSingleObject(handleListern, DEFAULT_WAIT_INTERVAL);
	//	if(eRevState == REV_NOT_HAPPEN)
	//	{
	//		WRITE_ERROR_LOG("线程等待时间结束，未能接收到服务器返回确认消息");
	//		printf("Warn: failed to recv comfirm data from server\n");
	//	}
	//	else
	//	{
	//		WRITE_INFO_LOG("接收到服务器返回的确认接收消息");
	//		if(eRevState == REV_YET_SUCCESS)
	//		{
	//			WRITE_WARN_LOG("服务器接收到数据与客户端发送的数据量不一致");
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
		WRITE_INFO_LOG("开始接收数据");
		DWORD start = GetCurMilliSec();
		DWORD terminal;
		if(m_interval > 0)
		{
			//Infor: 开启间隔数据显示线程
			hIntervalEvent = CreateEvent(NULL, false, false, LPCTSTR("Local/SendInterval"));
			m_hIntervalEvent = (HANDLE)_beginthreadex(NULL, 0, &TS_Interval, this, 0, NULL);
			if(m_hIntervalEvent == INVALID_HANDLE_VALUE)
			{
				WRITE_ERROR_LOG("创建间隔数据显示线程失败");
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
					WRITE_ERROR_LOG("服务器断开连接", WSAECONNRESET);
					printf("Server disconnects accidentally.\n");
				}
				else
				{
					WRITE_ERROR_LOG("接收数据失败", WSAGetLastError());
					printf("recv failed:( %d\n", WSAGetLastError());
				}
				break;
			}
			else if(res == sz_header)
			{
				memcpy(&header, m_pBuffer, sz_header);
				if(header.flag != '!')
				{
					WRITE_WARN_LOG("接收数据大小与尾指令相同");
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
				WRITE_INFO_LOG("接收数据完成");

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
			WRITE_WARN_LOG("客户端接收到数据与服务器发送的数据量不一致");
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


