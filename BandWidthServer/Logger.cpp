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
		str  = str + "：" + GetCustomError(errCode);
	}
	Trace(str);
}

string Logger::GetCustomError(int erroCode)
{
	string strMsg;
	switch(erroCode)
	{
	case WSAEINTR:
		strMsg = string("函数调用中断");
		break;
	case WSAEACCES:
		strMsg = string("权限被拒。尝试对套接字进行操作，但被禁止");
		break;
	case WSAEFAULT:
		strMsg = string("地址无效。传给Winsock函数的指针地址无效");
		break;
	case WSAENOTSOCK:
		strMsg = string("无效套接字上的套接字操作");
		break;
	case WSAEMSGSIZE:
		strMsg = string("消息过长");
		break;
	case WSAEADDRINUSE:
		strMsg = string("套接字地址正在使用");
		break;
	case WSAEADDRNOTAVAIL:
		strMsg = string("不能分配请求的地址");
		break;
	case WSAENETDOWN:
		strMsg = string("网络断开");
		break;
	case WSAENETUNREACH:
		strMsg = string("网络不可抵达");
		break;
	case WSAECONNABORTED:
		strMsg = string("网络造成连接取消");
		break;
	case WSAECONNRESET:
		strMsg = string("连接被对方重设");
		break;
	case WSAENOBUFS:
		strMsg = string("没有缓冲区空间");
		break;
	case WSAEISCONN:
		strMsg = string("套接字已经连接");
		break;
	case WSAENOTCONN:
		strMsg = string("套接字尚未连接");
		break;
	case WSAESHUTDOWN:
		strMsg = string("套接字关闭后不能发送");
		break;
	case WSAETIMEDOUT:
		strMsg = string("连接超时");
		break;
	case WSAECONNREFUSED:
		strMsg = string("连接被拒");
		break;
	case WSAEHOSTDOWN:
		strMsg = string("主机关闭");
		break;
	case WSAEHOSTUNREACH:
		strMsg = string("没有到主机的路由");
		break;
	case WSAEPROCLIM:
		strMsg = string("进程过多");
		break;
	case WSASYSNOTREADY:
		strMsg = string("网络子系统不可用");
		break;
	case WSAVERNOTSUPPORTED:
		strMsg = string("Winsock.dll版本有误");
		break;
	case WSANOTINITIALISED:
		strMsg = string("Winsock尚未初始化");
		break;
	case WSAEDISCON:
		strMsg = string("正在从容关闭");
		break;
	default:
		strMsg = string("未知错误 " + erroCode);
		break;
	}
	return strMsg;
}


/*获取当前调用该日志文件的程序路径或程序名称（无类型后缀），主用于日志名称声明
 *@param: [out] outFilePath 获取到程序路径或程序名称的字符串
 *        [in] sizeLen outFilePath字符数组的大小
 *        [in] bfetchName FALSE表示仅获取程序路径，TRUE表示获取程序名称
 *@return 如果获取成功则表示实际得到的字符串长度，0表示失败
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
