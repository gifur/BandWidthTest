// BandWidthServer.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"
#include "Server.h"
#include "Logger.h"

bool bPort = false;
int iport = DEFAULT_PORT;

void ValidateArgv(int argc, char** argv)
{
	for(int idx=1; idx<argc; idx++)
	{
		if(argv[idx][0] == '-')
		{
			switch(tolower(argv[idx][1]))
			{
			case 'p':
				if(idx + 1 < argc)
				{
					iport = atoi(argv[idx + 1]);
				}
				break;
			default:
				break;
			}
		}
	}
}

int _tmain(int argc, _TCHAR* argv[])
{
	ValidateArgv(argc, argv);
	Logger::InitLog();
	WRITE_INFO_LOG("----��������������----");
	WSADATA wsd;
	if(WSAStartup(MAKEWORD(2, 2), &wsd) != 0)
	{
		WRITE_ERROR_LOG("��ʼ��WinSock����", WSAGetLastError());
		cout << "Failed to load Winsock!" << endl;
		return 1;
	}
	Server* pServer = new Server(iport);
	if(pServer->Create() != SOCKET_ERROR)
	{
		pServer->Accept();
		WRITE_INFO_LOG("Ӧ�ó��������ر�\n");
	}
	WSACleanup();
	return 0;
}

