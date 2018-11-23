#include "stdafx.h"
#include "Logger.h"


Logger* Logger::m_pLogger = NULL;
CRITICAL_SECTION Logger::s_cs;

Logger::Logger(void)
{
	char fileName[MAX_PATH] = {0};
	if(GetCurExeNameOrPath(fileName, MAX_PATH, TRUE) != 0)
	{
		strcat_s(fileName, MAX_PATH, "_log.txt");
		m_fileName = fileName;
	}
	else
		m_fileName = DEFAULT_LOG_NAME;
}

Logger::~Logger(void)
{
	DeleteCriticalSection(&s_cs);
}

std::string Logger::GetCustomTime()
{
	char strTime[MAX_TIME_LEN];
	time_t now;
	time(&now);
	tm*pTm = new tm;
	localtime_s(pTm, &now);
	strftime(strTime, MAX_TIME_LEN, "[%Y/%m/%d  %H:%M:%S] ", pTm);
	return string(strTime);
}

void Logger::Trace(string strInfo)
{
	EnterCriticalSection(&s_cs);
	try
	{
		m_logStream.open(m_fileName.c_str(), fstream::in | fstream::app);
		if(!m_logStream)
		{

		}
		else if(m_logStream.is_open())
		{
			m_logStream << strInfo.c_str() << endl;
			m_logStream.close();
		}

	}
	catch (exception e)
	{
		LeaveCriticalSection(&s_cs);
	}
	LeaveCriticalSection(&s_cs);

	return;
}

Logger* Logger::GetInstance()
{
	EnterCriticalSection(&s_cs);
	if(m_pLogger == NULL)
	{
		m_pLogger = new Logger();
	}
	LeaveCriticalSection(&s_cs);
	return m_pLogger;
}

void Logger::InitLog()
{
	InitializeCriticalSection(&s_cs);
}

void Logger::TraceInfor(const char* strInfo)
{
	string str(GetCustomTime()+TRACE_INFOR);
	str += strInfo;
	Trace(str);
}

void Logger::TraceWarning(const char* strInfo, int wrnCode)
{
	string str(GetCustomTime()+TRACE_WARNNING);
	str += strInfo;
	Trace(str);
}

void Logger::TraceError(const char* strInfo, int errCode)
{
	string str(GetCustomTime()+TRACE_ERROR);
	str += strInfo;
	if(errCode != 0)
	{
		str  = str + "��" + GetCustomError(errCode);
	}
	Trace(str);
}

string Logger::GetCustomError(int erroCode)
{
	string strMsg;
	switch(erroCode)
	{
	case WSAEINTR:
		strMsg = string("���������ж�");
		break;
	case WSAEACCES:
		strMsg = string("Ȩ�ޱ��ܡ����Զ��׽��ֽ��в�����������ֹ");
		break;
	case WSAEFAULT:
		strMsg = string("��ַ��Ч������Winsock������ָ���ַ��Ч");
		break;
	case WSAENOTSOCK:
		strMsg = string("��Ч�׽����ϵ��׽��ֲ���");
		break;
	case WSAEMSGSIZE:
		strMsg = string("��Ϣ����");
		break;
	case WSAEADDRINUSE:
		strMsg = string("�׽��ֵ�ַ����ʹ��");
		break;
	case WSAEADDRNOTAVAIL:
		strMsg = string("���ܷ�������ĵ�ַ");
		break;
	case WSAENETDOWN:
		strMsg = string("����Ͽ�");
		break;
	case WSAENETUNREACH:
		strMsg = string("���粻�ɵִ�");
		break;
	case WSAECONNABORTED:
		strMsg = string("�����������ȡ��");
		break;
	case WSAECONNRESET:
		strMsg = string("���ӱ��Է�����");
		break;
	case WSAENOBUFS:
		strMsg = string("û�л������ռ�");
		break;
	case WSAEISCONN:
		strMsg = string("�׽����Ѿ�����");
		break;
	case WSAENOTCONN:
		strMsg = string("�׽�����δ����");
		break;
	case WSAESHUTDOWN:
		strMsg = string("�׽��ֹرպ��ܷ���");
		break;
	case WSAETIMEDOUT:
		strMsg = string("���ӳ�ʱ");
		break;
	case WSAECONNREFUSED:
		strMsg = string("���ӱ���");
		break;
	case WSAEHOSTDOWN:
		strMsg = string("�����ر�");
		break;
	case WSAEHOSTUNREACH:
		strMsg = string("û�е�������·��");
		break;
	case WSAEPROCLIM:
		strMsg = string("���̹���");
		break;
	case WSASYSNOTREADY:
		strMsg = string("������ϵͳ������");
		break;
	case WSAVERNOTSUPPORTED:
		strMsg = string("Winsock.dll�汾����");
		break;
	case WSANOTINITIALISED:
		strMsg = string("Winsock��δ��ʼ��");
		break;
	case WSAEDISCON:
		strMsg = string("���ڴ��ݹر�");
		break;
	default:
		strMsg = string("δ֪���� " + erroCode);
		break;
	}
	return strMsg;
}


/*��ȡ��ǰ���ø���־�ļ��ĳ���·����������ƣ������ͺ�׺������������־��������
 *@param: [out] outFilePath ��ȡ������·����������Ƶ��ַ���
 *        [in] sizeLen outFilePath�ַ�����Ĵ�С
 *        [in] bfetchName FALSE��ʾ����ȡ����·����TRUE��ʾ��ȡ��������
 *@return �����ȡ�ɹ����ʾʵ�ʵõ����ַ������ȣ�0��ʾʧ��
 */
DWORD Logger::GetCurExeNameOrPath(char* outFilePath, int sizeLen, BOOL bfetchName/* = FALSE */)
{
	DWORD tmpDwRes = 0;
	if(sizeLen <= 0) 
		return 0;
	memset(outFilePath, 0, sizeof(char)*sizeLen);
	if(tmpDwRes = GetModuleFileName(NULL, outFilePath, sizeLen) == 0)
	{
		return 0;
	}
	if(!bfetchName)
	{
		return tmpDwRes;
	}
	char* pch1 = strrchr(outFilePath, '\\');
	char* pch2 = strrchr(outFilePath, '.');
	if(pch1 == NULL || pch2 == NULL)
		return 0;
	char tmpFilePath[MAX_PATH] = {0};
	char* pstart = pch1 + 1;
	int idx=0;
	for(; pstart < pch2; idx++)
	{
		tmpFilePath[idx] = *pstart++;
	}
	tmpFilePath[idx] = '\0';
	memcpy_s(outFilePath, sizeof(char)*sizeLen, tmpFilePath, sizeof(char)*(idx+1));
	return idx+1;


}

void Logger::Trace_format(const char* fmt, va_list list_arg)
{
	int n;
	n = _vscprintf(fmt, list_arg);
	if(n > 0)
	{
		char *buf = new char[n+1];
		memset(buf, 0, sizeof(char)*(n+1));
		vsprintf_s(buf, n+1, fmt, list_arg);
		Trace(string(buf));
		delete buf;
		buf = NULL;
	}
	va_end(list_arg);
}

void Logger::TraceInfor_f(const char* fmt, ...)
{
	string str(GetCustomTime()+TRACE_INFOR + fmt);
	va_list arg;
	va_start(arg, fmt);
	Trace_format(str.c_str(), arg);
}

void Logger::TraceWarning_f(const char* fmt, ...)
{
	string str(GetCustomTime()+TRACE_WARNNING + fmt);
	va_list arg;
	va_start(arg, fmt);
	Trace_format(str.c_str(), arg);
}

void Logger::TraceError_f(const char* fmt, ...)
{
	string str(GetCustomTime()+TRACE_ERROR + fmt);
	va_list arg;
	va_start(arg, fmt);
	Trace_format(str.c_str(), arg);
}
