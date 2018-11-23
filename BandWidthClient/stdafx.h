// stdafx.h : 标准系统包含文件的包含文件，
// 或是经常使用但不常更改的
// 特定于项目的包含文件
//

#pragma once

#include "targetver.h"

//
#include <iostream>
#include <iomanip>
#include <fstream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <tchar.h>
#include <stdarg.h>
#include <string>
#include <vector>
#include <assert.h>

#pragma  comment(lib, "ws2_32.lib")

using namespace std;

#define DEFAULT_PORT 5150
#define DEFAULT_BUFSZE 8192

#define DEFAULT_MAX_SEND 1024

#define SOCKET_CUS_TCP 100
#define SOCKET_CUS_UDP 101

#define DEFAULT_CNT_NUM 1000
#define MAX_DEFAULT_THREAD_NUM 64
#define DEFAULT_SEND_NUM 0
#define DEFAULT_SEND_TIME 10  // sec

#define DEFAULT_INTERVAL_TIME 0 // s

typedef struct Option{
	char format;
	int interval;
	int len;
	int port;
	int time;
	int bufsize;
	int num;
	char szServer[128];
	bool bUpload;
	Option(){
		ResetOption();
	}
	void ResetOption()
	{
		format = 'a';
		interval = DEFAULT_INTERVAL_TIME;
		len = DEFAULT_BUFSZE;
		port = DEFAULT_PORT;
		time = DEFAULT_SEND_TIME;
		num = DEFAULT_SEND_NUM;
		bUpload = true;
		bufsize = DEFAULT_BUFSZE;
		strcpy_s(szServer, "127.0.0.1");
	}
}option_settings;

typedef struct client_hdr{
	int isUpload;
	int bufsize;
	signed int m_amount;
}client_hdr;

typedef struct client_tail{
	char tail_data[14];
	client_tail()
	{
		memset(tail_data, 0, 14);
		tail_data[4] = '9';
	}
}client_tail;

typedef struct server_hdr{
	int m_amount;
	int use_time; 
	char flag;
	server_hdr(){
		flag = '!';
	}
}server_hdr;

const char usage_help[] = "\
	BandWidthClient -c host [options]\n\
	\n\
	-i  #          每次显示实时上传/下载速率的间隔秒数\n\
	-p  5150       连接服务器的端口号\n\
	-u             [默认]上传数据模式下测试速率\n\
	-d             下载数据模式下测试速率\n\
	-n  #[kKmM]    指定发送或接收的数据量，通过耗时计算速率\n\
	-t  10         [默认]指定发送或接收数据时间，通过数据量计算速率\n\
	-s  8192       每次发送/接收数据的缓冲区大小\n\n";

// TODO: 在此处引用程序需要的其他头文件
