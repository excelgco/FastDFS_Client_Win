#ifndef COMMON_DEFINE_
#define COMMON_DEFINE_

#include <stdio.h>
#include <TCHAR.h>

#define FDFS_PROTO_PKG_LEN_SIZE 8
#define DEFAULT_NETWORK_TIMEOUT 60000
#define TRACKER_QUERY_STORAGE_STORE_BODY_LEN 40

#define FDFSC_DEBUG_MODE 0
#define FDFSC_INFO_MODE 1
#define FDFSC_WARNING_MODE 2
#define FDFSC_ERROR_MODE 3

#define LogFileName _T("FDFSC_log.txt")
#define IP_ADDRESS_SIZE 16

typedef struct
{
	int sock;
	int port;
	TCHAR ip_addr[IP_ADDRESS_SIZE];
	CRITICAL_SECTION csSend;
	CRITICAL_SECTION csRecv;
} ConnectionInfo;

extern int g_nLogLevel;

void __cdecl WriteLogInfo(const TCHAR * pszFileName, int nLogLevel, const TCHAR * pszLogInfo, ...);

UINT32 buff2int(const char *buff);
void __cdecl long642buff(UINT64 n, char *buff);
UINT64 __cdecl buff2long64(const char *buff);
#endif