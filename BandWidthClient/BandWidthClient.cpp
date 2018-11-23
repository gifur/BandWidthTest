// BandWidthClient.cpp : 定义控制台应用程序的入口点。

#include "stdafx.h"
#include "Client.h"
#include "Logger.h"

option_settings ValidateArgv(int argc, char** argv)
{
	option_settings settings;
	bool torn = false;
	for(int idx = 1; idx < argc; idx++)
	{
		if(argv[idx][0] == '-')
		{
			switch(tolower(argv[idx][1]))
			{
			case 'u':
				settings.bUpload = true;
				break;
			case 'd':
				settings.bUpload = false;
				break;
			case 'c':
				if(idx + 1 < argc)
				{
					strcpy_s(settings.szServer, argv[idx + 1]);
				}
				else
				{
					WRITE_WARN_LOG("参数输入不完整");
				}
				break;
			case 'i':
				if(idx + 1 < argc)
				{
					settings.interval = atoi(argv[idx + 1]);
				}
				else
				{
					WRITE_WARN_LOG("参数输入不完整");
				}
				break;
			case 't':
				if(idx + 1 < argc && !torn)
				{
					torn = true;
					settings.time = atoi(argv[idx + 1]);
				}
				else if(!torn)
				{
					WRITE_WARN_LOG("参数输入不完整");
				}
				break;
			case 'n':
				if(idx + 1 < argc && !torn)
				{
					torn = true;
					int iNum = 0;
					char suffix = '\0';
					sscanf_s(argv[idx+1], "%d%c", &iNum, &suffix);
					switch(suffix)
					{
					case 'K': iNum *= 1024; break;
					case 'M': iNum *= 1024*1024; break;
					case 'k': iNum *= 1000 / 8; break;
					case 'm': iNum *= 1000 * 1000 / 8; break;
					default: 
						break;
					}
					settings.num = iNum;
					settings.time = 0;
				}
				else if(!torn)
				{
					WRITE_WARN_LOG("参数输入不完整");
				}
				break;
			case 's':
				if(idx + 1 < argc)
				{
					settings.bufsize = atoi(argv[idx + 1]);
				}
				else
				{
					WRITE_WARN_LOG("参数输入不完整");
				}
				break;
			case 'p':
				if(idx + 1 < argc)
				{
					settings.port = atoi(argv[idx + 1]);
				}
				break;
			case 'h':
				{
					printf(usage_help);
					exit(0);
				}
				break;
			case 'f':
				if(idx + 1 < argc)
				{
					settings.format = argv[idx + 1][0];
				}
				break;
			default:
				WRITE_WARN_LOG("参数输入未能识别");
				break;
			}
		}
	}
	return settings;
}

int _tmain(int argc, _TCHAR* argv[])
{
	Logger::InitLog();
	WRITE_INFO_LOG("----程序开始运行----");
	option_settings settings = ValidateArgv(argc, argv);
	WSADATA wsd;
	if(WSAStartup(MAKEWORD(2, 2), &wsd) != 0)
	{
		WRITE_ERROR_LOG("初始化 Winsock出错");
		printf("Failed to load Winsock!\n");
		exit(-1);
	}
	Client* pClient;
	pClient = new Client();
	if(pClient->Connect(settings.szServer, settings.port) == SOCKET_ERROR)
	{
		goto GtEnd;
	}
	pClient->InitSettings(settings);
	if(settings.bUpload)
	{
		pClient->StartSend();
	}
	else
	{
		pClient->StartRev();
	}

GtEnd:
	WSACleanup();
	WRITE_INFO_LOG("应用程序关闭\n");
	system("pause");
	return 0;
}