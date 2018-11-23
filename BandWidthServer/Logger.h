#pragma once

#include "stdafx.h"

#define MAX_INFOR_LEN 4096
#define MAX_TIME_LEN 128
#define DEFALUT_INFOR_LEN 2048

#define WRITE_INFO_LOG (Logger::GetInstance()->TraceInfor)
#define WRITE_WARN_LOG (Logger::GetInstance()->TraceWarning)
#define WRITE_ERROR_LOG (Logger::GetInstance()->TraceError)

#define WRITE_INFO_PARAM (Logger::GetInstance()->TraceInfor_f)
#define WRITE_WARN_PARAM (Logger::GetInstance()->TraceWarning_f)
#define WRITE_ERROR_PARAM (Logger::GetInstance()->TraceError_f)

const string DEFAULT_LOG_NAME = "Log.txt";

const string TRACE_INFOR = "Infor: ";
const string TRACE_WARNNING = "Warning: ";
const string TRACE_ERROR = "Error: ";

class Logger
{
public:
	~Logger(void);

	static void InitLog();
	static Logger* GetInstance();
	static CRITICAL_SECTION s_cs;

	void TraceInfor(const char* strInfo);
	void TraceWarning(const char* strInfo, int wrnCode = 0);
	void TraceError(const char* strInfo, int errCode = 0);

	void TraceInfor_f(const char* fmt, ...);
	void TraceWarning_f(const char* fmt, ...);
	void TraceError_f(const char* fmt, ...);

	void SetFileName(string val);
	string GetFileName();

	string GetCustomError(int erroCode);

private:
	Logger();
	static Logger* m_pLogger;
	string m_fileName;
	fstream m_logStream;
	string GetCustomTime();
	void Trace(string strInfo);
	void Trace_format(const char* fmt, va_list list_arg);
	DWORD GetCurExeNameOrPath(char* outFilePath, int sizeLen, BOOL bfetchName = FALSE);
};

