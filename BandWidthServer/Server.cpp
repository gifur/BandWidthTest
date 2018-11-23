#include "stdafx.h"
#include "Server.h"
#include "InnerClient.h"
#include "Logger.h"
#include "process.h"

unsigned int Server::ClientThread(LPVOID lpParam)
{
	char ch[DEFALUT_INFOR_LEN] = {0};
    SOCKET client = (SOCKET)lpParam;
    InnerClient*  pClient = new  InnerClient(client);
	int res = 0;
	client_hdr header;

	//Infor: 接收新连接的客户端的接下来要执行内容的命令
	res = recv(client, (char *)&header, sizeof(header), 0);
	if(res != sizeof(header))
	{
		if(res == SOCKET_ERROR)
		{
			sprintf_s(ch, "[%s] 首命令接收失败", pClient->toString().c_str());
			WRITE_ERROR_LOG(ch, WSAGetLastError());
		}
		pClient->CloseSocket();
		return -1;
	}
    pClient->InitByHeader(header);
    bool bRecv = (bool)header.isUpload;
	
	if(bRecv)
	{//接收
		sprintf_s(ch, "[%s] 开始接收数据", pClient->toString().c_str());
		WRITE_INFO_LOG(ch);
		pClient->Recv();
	}
	else
	{//发送
		sprintf_s(ch, "[%s] 开始发送数据", pClient->toString().c_str());
		WRITE_INFO_LOG(ch);
		pClient->Send();
	}
	Sleep(500);
	pClient->CloseSocket();
    return 0;
}

Server::Server(int iPort):m_pBuffer(NULL),m_iPort(iPort)
{
    m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    m_socketAddr.sin_family = AF_INET;
    m_socketAddr.sin_port = htons(m_iPort);
    m_socketAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    m_curThreadNum = 0;
}

int Server::Create()
{
    int ret = bind(m_socket, (PSOCKADDR)&m_socketAddr, sizeof(m_socketAddr));
    if(ret == SOCKET_ERROR)
    {
		WRITE_ERROR_LOG("绑定套接字失败", WSAGetLastError());
        cout << "bind() failed : " << WSAGetLastError() << endl;
        return SOCKET_ERROR;
    }
	else
	{
		listen(m_socket, DEFAULT_CNT_NUM);
		char ch[DEFALUT_INFOR_LEN] = {0};
		sprintf_s(ch, "服务器开始监听，端口号为 %d", m_iPort);
		WRITE_INFO_LOG(ch);
		printf("------------------------------------------------------------\n");
		printf("Server listening on TCP port %d\n", m_iPort);
		printf("------------------------------------------------------------\n");
	}
    return ret;
}

int Server::Accept()
{
    SOCKET sClient;
    SOCKADDR_IN clientAddr;
    HANDLE threadHandle;
	unsigned int threadId;
	char ch[DEFALUT_INFOR_LEN] = {0};
    while(true)
    {
        int iClientSize = sizeof(clientAddr);
        sClient = accept(m_socket, (LPSOCKADDR)&clientAddr, &iClientSize);
        if(sClient == INVALID_SOCKET)
        {
			WRITE_ERROR_LOG("接收客户端SOCKET出错", WSAGetLastError());
            cout << "accept() failed: " << WSAGetLastError() << endl;
            continue;
        }
        cout << "Accept client from " << inet_ntoa(clientAddr.sin_addr) << " port " << ntohs(clientAddr.sin_port) << endl;
		sprintf_s(ch, "[%s:%d] 新客户连接服务器", inet_ntoa(clientAddr.sin_addr),  ntohs(clientAddr.sin_port));
		WRITE_INFO_LOG(ch);
        threadHandle = (HANDLE)_beginthreadex(NULL, 0, ClientThread, (LPVOID)sClient, 0, &threadId);
        if(threadHandle == INVALID_HANDLE_VALUE)
        {
			//Warning: 创建连接客户端操作的线程失败 
			sprintf_s(ch, "[%s:%d] 为客户端分配线程失败", inet_ntoa(clientAddr.sin_addr),  ntohs(clientAddr.sin_port));
			WRITE_ERROR_LOG(ch, WSAGetLastError());
            cout << "Create Recv thread failed: " << WSAGetLastError() << endl;
			closesocket(sClient);
        }
		else
		{
			sprintf_s(ch, "[%s:%d] 线程号：%d", 
				inet_ntoa(clientAddr.sin_addr),  ntohs(clientAddr.sin_port), threadId);
			WRITE_INFO_LOG(ch);
		}
    }
    return 0;
}

Server::~Server()
{
	if(m_socket != INVALID_SOCKET)
	{
		shutdown(m_socket, SD_BOTH);
		closesocket(m_socket);
	}
}

int Server::Run()
{
	char ch[DEFALUT_INFOR_LEN] = {0};
	int res;

	ULONG nonblock = 1;
	FD_SET fdRead;
	FD_SET fdWrite;
	clientInfor.clear();

	SOCKET client;
	SOCKADDR_IN clientAddr;
	int szAddrSize = sizeof(clientAddr);

	ioctlsocket(m_socket, FIONBIO, &nonblock);

	while (true)
	{
		FD_ZERO(&fdRead);
		FD_ZERO(&fdWrite);

		FD_SET(m_socket, &fdRead);
		for (vector<InnerClient>::const_iterator iter = clientInfor.begin(); iter != clientInfor.end(); iter++)
		{
			FD_SET(iter->m_socket, &fdRead);
			FD_SET(iter->m_socket, &fdWrite);
		}

		if((res = select(0, &fdRead, &fdWrite, NULL, NULL)) == SOCKET_ERROR)
		{
			WRITE_ERROR_LOG("调用select方法失败", WSAGetLastError());
			break;
		}
		if(res == 0) continue;

		if(FD_ISSET(m_socket, &fdRead))
		{
			client = accept(m_socket, (LPSOCKADDR)&clientAddr, &szAddrSize);
			if(client == INVALID_SOCKET)
			{
				WRITE_ERROR_LOG("接收客户端SOCKET出错", WSAGetLastError());
				cout << "accept() failed: " << WSAGetLastError() << endl;
				continue;
			}
			cout << "Accept client from " << inet_ntoa(clientAddr.sin_addr) << " port " << ntohs(clientAddr.sin_port) << endl;
			sprintf_s(ch, "新客户：%s:%d 连接服务器", inet_ntoa(clientAddr.sin_addr),  ntohs(clientAddr.sin_port));
			WRITE_INFO_LOG(ch);

			ioctlsocket(client, FIONBIO, &nonblock);
			InnerClient clientInfo(client);
			clientInfor.push_back(clientInfo);
		}
		for(size_t i=0; i<clientInfor.size(); i++)
		{
			if(FD_ISSET(clientInfor[i].m_socket, &fdRead))
			{
				InnerClient& curClient = clientInfor[i];
				if(curClient.m_status == STATUS_CLIENT_IDLE)
				{
					if(curClient.PerformRevCmd() == -1)
					{

					}

				}
				else if(curClient.m_status == STATUS_CLIENT_REV)
				{
					if(curClient.PerformRevData() == -2)
					{
						//退出服务器
					}
				}
			}
			if(FD_ISSET(clientInfor[i].m_socket, &fdWrite))
			{
				InnerClient& curClient = clientInfor[i];
				if(curClient.m_status == STATUS_CLIENT_IDLE)
				{

				}
			}
		}
	}
	return 0;
}
