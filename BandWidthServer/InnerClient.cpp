#include "stdafx.h"
#include "InnerClient.h"
#include "Logger.h"
#include "Format.h"
#include <assert.h>

InnerClient::InnerClient(SOCKET val):m_pBuffer(NULL)
{
    m_socket = val;
    port = -1;
	m_stAmout = 0;
	m_stTime = 0;
	m_aimTime = 0;
	m_aimAmout = 0;
	//////////////////////////////////////////////////////////////////////////
	m_status = STATUS_CLIENT_IDLE;
}

string InnerClient::toString()
{
    if(port == -1)
    {
        char szText[128];
        SOCKADDR_IN peerAddr;
        int peerSize = sizeof(peerAddr);
        getpeername(m_socket, (SOCKADDR*)&peerAddr, &peerSize);
        memset(szText, 0, 128);
        port = ntohs(peerAddr.sin_port);
        sprintf_s(szText, 128, "%s:%d", inet_ntoa(peerAddr.sin_addr), port);
        strIp = string(szText);
    }
    return strIp;
}

void InnerClient::Recv()
{
	char ch[DEFALUT_INFOR_LEN] = {0};
	int res;
	client_tail tail;
	int sz_tail = sizeof(tail);
	DWORD start = GetCurMilliSec();
	DWORD terminal;
	//if(isTimeEvn)
	{
		while(true)
		{
			res = recv(m_socket, m_pBuffer, m_bufSize, 0);
			if(res == 0) break;
			else if(res == SOCKET_ERROR)
			{
				if(WSAGetLastError() == WSAECONNRESET)
				{
					sprintf_s(ch, "[%s] 客户端异常关闭", toString().c_str());
					WRITE_ERROR_LOG(ch, WSAGetLastError());
					printf("Client from %s disconnects accidentally.\n", toString().c_str());
					//cout << "client from " << toString() << " disconnects accidentally." << endl;
				}
				else
				{
					sprintf_s(ch, "[%s] 本地接收失败", toString().c_str());
					WRITE_ERROR_LOG(ch, WSAGetLastError());
					printf("%s Recv failed with status %d\n", toString().c_str(), WSAGetLastError());
					//cout << "recv failed : " << WSAGetLastError() << endl;
				}
				break;
			}
			else if(res == sz_tail)
			{
				memcpy(&tail, m_pBuffer, sz_tail);
				if(tail.tail_data[4] != '9')
				{
					m_stAmout += res;
					terminal = GetCurMilliSec();
					continue;
				}
				sprintf_s(ch, "[%s] 收到确认信息，结束接收数据", toString().c_str());
				WRITE_INFO_LOG(ch);
				m_stTime = terminal - start;
				double fSentSize = (double)m_stAmout;
				double dSentTime = (double)m_stTime;

				assert(m_stTime != 0);

				server_hdr s_header;
				s_header.m_amount = m_stAmout;
				s_header.use_time = m_stTime;
				res = send(m_socket, (const char*)&s_header, sizeof(s_header), 0);
				if(res != sizeof(s_header))
				{
					printf("%s : Send revc-tail failed with status %d\n", toString().c_str(), WSAGetLastError());
					sprintf_s(ch, "[%s] 发送确认数据回客户端失败", toString().c_str());
					WRITE_WARN_LOG(ch);
				}
				PrintResult();
				break;
			}
			else
			{
				m_stAmout += res;
				terminal = GetCurMilliSec();
			}
		}
	}
}

void InnerClient::PrintResult(BOOL isRecv)
{
	char buffer[65] = {0};
	char bufferWrite[128] = {0};
	double dSentSize = (double)m_stAmout;
	double dSentTime = m_stTime > 0 ? (double)m_stTime : 1.0;
	double dTotalNum = dSentSize;
	double dTotalTime = dSentTime / 1000;
	ByteSprintf(buffer, 32, dTotalNum, 'A');
	ByteSprintf(buffer+32, 32, dTotalNum / dTotalTime, 'a');
	if(isRecv)
	{
		if(isTimeEvn)
		{
			printf(result_t_receive, toString().c_str(), 0.0, dTotalTime, buffer, buffer+32);
		}
		else
		{
			printf(result_n_receive, toString().c_str(), 0.0, dTotalTime, buffer, buffer+32);
		}
	}
	else
	{
		if(isTimeEvn)
		{
			printf(result_t_send, toString().c_str(), 0.0, dTotalTime, buffer, buffer+32);
		}
		else
		{
			printf(result_n_send, toString().c_str(), 0.0, dTotalTime, buffer, buffer+32);
		}
	}
}

void InnerClient::InitByHeader(client_hdr val)
{
	if(val.m_amount > 0)
	{
		isTimeEvn = true;
		m_aimTime = val.m_amount;
	}
	else if(val.m_amount < 0)
	{
		isTimeEvn = false;
		m_aimAmout = -val.m_amount;
	}
	m_bufSize = val.bufsize;
	if(m_pBuffer != NULL)
	{
		delete m_pBuffer;
		m_pBuffer = NULL;
	}
	m_pBuffer = new char[m_bufSize];

	//////////////////////////////////////////////////////////////////////////
	if(val.isUpload == 1)
		m_status = STATUS_CLIENT_REV;
	else
		m_status = STATUS_CLIENT_SEND;
}

void InnerClient::Send()
{
	char ch[DEFALUT_INFOR_LEN] = {0};
	int res;
	DWORD start = GetCurMilliSec();
	DWORD terminal;
	if(isTimeEvn)
	{//根据时间长度进行发送
		DWORD breakout = m_aimTime * 1000;
		while(true)
		{
			res = send(m_socket, m_pBuffer, m_bufSize, 0);
			if(res == SOCKET_ERROR)
			{
				break;
			}
			m_stAmout += res;
			terminal = GetCurMilliSec();
			m_stTime = terminal - start;
			if(m_stTime >= breakout) break;
		}
	}
	else
	{//根据数据量进行发送
		int breakAmont = m_aimAmout;
		int sendSize = m_bufSize > m_aimAmout ? m_aimAmout : m_bufSize;
		while (true)
		{
			res = send(m_socket, m_pBuffer, sendSize, 0);
			if(res == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK)
			{
				break;
			}
			m_stAmout += res;
			breakAmont -= res;
			if(breakAmont <= 0)
			{
				m_stTime = GetCurMilliSec() - start;
				break;
			}
			sendSize = m_bufSize > breakAmont ? breakAmont : m_bufSize;
		}
	}
	if(res == SOCKET_ERROR)
	{
		sprintf_s(ch, "[%s] 本地发送失败", toString().c_str());
		WRITE_ERROR_LOG(ch, WSAGetLastError());
		return;
	}
	//发送服务器头部
	server_hdr header;
	header.m_amount = m_stAmout;
	header.use_time = m_stTime;
	int sSize = sizeof(header);
	res = send(m_socket, (const char*)&header, sSize, 0);
	if(res == SOCKET_ERROR)
	{
		sprintf_s(ch, "[%s] 本地发送结束指令失败", toString().c_str());
		WRITE_ERROR_LOG(ch, WSAGetLastError());
	}
	else if(res != sSize)
	{
		sprintf_s(ch, "[%s] 本地发送结束指令不完整", toString().c_str());
		WRITE_ERROR_LOG(ch, WSAGetLastError());
	}
	PrintResult(FALSE);
	return;
}

void InnerClient::CloseSocket()
{
	if(m_socket != INVALID_SOCKET)
	{
		shutdown(m_socket, SD_BOTH);
		closesocket(m_socket);
	}
	return;
}

//////////////////////////////////////////////////////////////////////////

int InnerClient::PerformRevCmd()
{
	int res;
	char ch[DEFALUT_INFOR_LEN] = {0};

	client_hdr header;
	res = recv(m_socket, (char*)&header, sizeof(header), 0);
	if(res != sizeof(header))
	{
		if(res == SOCKET_ERROR)
		{
			sprintf_s(ch, "[%s] 首命令接收失败", toString().c_str());
			WRITE_ERROR_LOG(ch, WSAGetLastError());
		}
		return -1;
	}
	InitByHeader(header);
	return 0;
}

int InnerClient::PerformRevData()
{
	int res;
	int iTailSize = sizeof(client_tail);
	char ch[DEFALUT_INFOR_LEN] = {0};
	res = recv(m_socket, m_pBuffer, m_bufSize, 0);
	if(res == SOCKET_ERROR && WSAGetLastError() == WSAECONNRESET)
	{
		sprintf_s(ch, "[%s] 客户端异常关闭", toString().c_str());
		WRITE_ERROR_LOG(ch, WSAGetLastError());
		cout << "client from " << toString() << " disconnects accidentally." << endl;
		return -2;
	}
	else if(res == iTailSize)
	{
		sprintf_s(ch, "[%s] 结束接收数据", toString().c_str());
		WRITE_INFO_LOG(ch);
	
		m_status = STATUS_CLIENT_IDLE;
	}
	else if(res == 0)
	{
		return -1;
	}
	m_stAmout += res;
	return 0;
}


