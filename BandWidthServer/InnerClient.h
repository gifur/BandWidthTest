#pragma once

using namespace std;

#define STATUS_CLIENT_IDLE 1
#define STATUS_CLIENT_REVCMD 2
#define STATUS_CLIENT_BUSY 3

#define STATUS_CLIENT_SEND 4
#define STATUS_CLIENT_REV 5

class InnerClient
{
public:
    InnerClient(SOCKET val);
    SOCKET m_socket;
    string toString();
	void InitByHeader(client_hdr val);

	bool isTimeEvn;

	//////////////////////////////////////////////////////////////////////////
	int m_status;
	int PerformRevCmd();
	int PerformRevData();
	int PerformSendData();

	int m_stAmout;
	int m_aimAmout;
	DWORD m_aimTime;
	DWORD m_stTime;

	void Recv();
	void Send();

	void CloseSocket();

private:
    string strIp;
    int port;

	char *m_pBuffer;
	int m_bufSize;

	void PrintResult(BOOL isRecv = TRUE);
	DWORD GetCurMilliSec()
	{
		return (DWORD)clock();
	}
};
