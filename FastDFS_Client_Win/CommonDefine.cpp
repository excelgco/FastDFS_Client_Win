#include <stdarg.h>
#include <time.h>
#include <windows.h>
#include <io.h>
#include <direct.h>
#include "CommonDefine.h"

#define LOG_FILE_MAX_SIZE					(1024 * 1024 * 50)
#define LOG_INFO_MAX_SIZE					(1024 * 24)

UINT32 buff2int(const char *buff)
{
	return  (((unsigned char)(*buff)) << 24) | \
		(((unsigned char)(*(buff+1))) << 16) |  \
		(((unsigned char)(*(buff+2))) << 8) | \
		((unsigned char)(*(buff+3)));
}

UINT64 __cdecl buff2long64(const char *buff)
{
	unsigned char *p;
	p = (unsigned char *)buff;
	return  (((UINT64)(*p)) << 56) | \
		(((UINT64)(*(p+1))) << 48) |  \
		(((UINT64)(*(p+2))) << 40) |  \
		(((UINT64)(*(p+3))) << 32) |  \
		(((UINT64)(*(p+4))) << 24) |  \
		(((UINT64)(*(p+5))) << 16) |  \
		(((UINT64)(*(p+6))) << 8) | \
		((UINT64)(*(p+7)));
}

void __cdecl long642buff(UINT64 n, char *buff)
{
	unsigned char *p;
	p = (unsigned char *)buff;
	*p++ = (n >> 56) & 0xFF;
	*p++ = (n >> 48) & 0xFF;
	*p++ = (n >> 40) & 0xFF;
	*p++ = (n >> 32) & 0xFF;
	*p++ = (n >> 24) & 0xFF;
	*p++ = (n >> 16) & 0xFF;
	*p++ = (n >> 8) & 0xFF;
	*p++ = n & 0xFF;
}

void __stdcall GetExePath(TCHAR* pBuf, int nBufSize)
{
	HMODULE				hDll;
	TCHAR				szPath[MAX_PATH];
	TCHAR*				p;

	hDll = GetModuleHandle(NULL);
	GetModuleFileName(hDll, szPath, MAX_PATH);
	p	= _tcsrchr(szPath, _T('\\'));
	*p	= _T('\0');
	_tcscpy_s(pBuf, nBufSize, szPath);
}

static void __stdcall WriteFileInternal(const TCHAR * szFilePathname, DWORD dwFileMaxSize, TCHAR * szInfo)
{
	HANDLE	hFile;
	DWORD	dwFileSize, dwBytesWritten;
	
	hFile = CreateFile(szFilePathname, GENERIC_WRITE, FILE_SHARE_WRITE|FILE_SHARE_READ, 0, OPEN_ALWAYS, 0, 0);
	if(hFile == INVALID_HANDLE_VALUE)
		return;

	dwFileSize = GetFileSize(hFile, NULL);
	if(dwFileSize >= dwFileMaxSize)
	{
		CloseHandle(hFile);
		hFile = CreateFile(szFilePathname, GENERIC_WRITE, FILE_SHARE_WRITE|FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);
		if(hFile == INVALID_HANDLE_VALUE)
			return;	
	}

	SetFilePointer(hFile, 0, 0, FILE_END);
	WriteFile(hFile, szInfo, (DWORD)_tcslen(szInfo) * sizeof(TCHAR), &dwBytesWritten, NULL);
	CloseHandle(hFile);
}



void __cdecl WriteLogInfo(const TCHAR * pszFileName, int nLogLevel, const TCHAR * pszLogInfo, ...)
{
	if(nLogLevel < g_nLogLevel)
	{
		return;
	}

	TCHAR			szLogInfoTmp[LOG_INFO_MAX_SIZE];
	TCHAR			szLogInfoFull[LOG_INFO_MAX_SIZE + 64];
	TCHAR			szPath[MAX_PATH];
	TCHAR			szLogFolder[MAX_PATH];
	TCHAR			szFullPathName[MAX_PATH];
	TCHAR			szDate[16];
	TCHAR			szTime[16];
	SYSTEMTIME		sysTime;
	va_list			args;

	if(pszFileName == NULL || pszFileName[0] == _T('\0') || pszLogInfo == NULL || pszLogInfo[0] == _T('\0') 
		|| _tcslen(pszLogInfo) >= LOG_INFO_MAX_SIZE)
		return;

	_tstrdate_s(szDate, 16);
	_tstrtime_s(szTime, 16);

	va_start(args, pszLogInfo);
	_vstprintf_s(szLogInfoTmp, LOG_INFO_MAX_SIZE, pszLogInfo, args);
	va_end(args);

	GetExePath(szPath, MAX_PATH);
	_stprintf_s(szLogFolder, MAX_PATH, _T("%s\\log"), szPath);

	if(_access(szLogFolder, 0))
	{
		_mkdir(szLogFolder);
	}

	GetLocalTime(&sysTime);

	_stprintf_s(szLogInfoFull, LOG_INFO_MAX_SIZE + 64, _T("%s %s %s\r\n"), szDate, szTime, szLogInfoTmp);
	_stprintf_s(szFullPathName, MAX_PATH, _T("%s\\%04d%02d%02d_%s"), szLogFolder, sysTime.wYear, sysTime.wMonth, sysTime.wDay, pszFileName);

	WriteFileInternal(szFullPathName, LOG_FILE_MAX_SIZE, szLogInfoFull);
}
