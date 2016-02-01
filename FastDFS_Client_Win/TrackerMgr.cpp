#include <winsock2.h>
#include <windows.h>
#include "CommonDefine.h"
#include "TrackerMgr.h"
#include "FDFS_Packet.h"
#include "Sockopt.h"

#pragma comment(lib, "WS2_32.lib")

TrackerMgr::TrackerMgr()
{
	InitializeCriticalSection(&m_csOperation);
	m_nLoop = 0;
}

TrackerMgr::~TrackerMgr()
{
	DeleteCriticalSection(&m_csOperation);
}

UINT32 TrackerMgr::Initialize(ServerAddress *pAddr, UINT32 nArrCount)
{
	UINT32 nRet;
	UINT32 nSuccess = 0;

	ServerAddress *pCurrentAddr = pAddr;
	nSuccess = 0;
	int i;
	for(i = 0; i < nArrCount; i++)
	{
		ConnectionInfo *pCurrentConn = new ConnectionInfo();
		pCurrentConn->port = pCurrentAddr->nPort;
		strcpy(pCurrentConn->ip_addr, (char*)pCurrentAddr->szIP);
		pCurrentConn->sock = INVALID_SOCKET;
		InitializeCriticalSection(&pCurrentConn->csRecv);
		InitializeCriticalSection(&pCurrentConn->csSend);
		nRet = ConnectToTracker(pCurrentConn);
		if(nRet != enumSuccess_FDFS)
		{
			WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("TrackerMgr::Initialize Connect Failed IP:%s, Port:%d, Return Code:%d"), pCurrentConn->ip_addr, pCurrentConn->port, nRet);
			DeleteCriticalSection(&pCurrentConn->csRecv);
			DeleteCriticalSection(&pCurrentConn->csSend);
			delete pCurrentConn;
		}
		else
		{
			nSuccess++;
			m_deqConnectionInfo.push_back(pCurrentConn);
		}
		pCurrentAddr++;
	}

	if(nSuccess > 0 || (pAddr == NULL || nArrCount == 0))
		return enumSuccess_FDFS;
	else
		return enumFailure_FDFS;
}

void TrackerMgr::UnInitialize()
{
	EnterCriticalSection(&m_csOperation);
	int	nDeqSize = (int)m_deqConnectionInfo.size();
	for(int i = 0; i < nDeqSize; i++)
	{
		ConnectionInfo *pConn = m_deqConnectionInfo[i];
		if(pConn)
		{
			if(pConn->sock != INVALID_SOCKET)
			{
				closesocket(pConn->sock);
				pConn->sock = INVALID_SOCKET;
			}
			DeleteCriticalSection(&pConn->csRecv);
			DeleteCriticalSection(&pConn->csSend);

			delete pConn;
			pConn = NULL;
		}
	}
	m_deqConnectionInfo.clear();
	LeaveCriticalSection(&m_csOperation);
	return;
}

//简单的轮询
ConnectionInfo* TrackerMgr::GetConnection()
{
	ConnectionInfo *pRet = NULL;
	int	nDeqSize	= (int)m_deqConnectionInfo.size();
	if(nDeqSize == 0)
		return NULL;
	if(m_nLoop < nDeqSize)
		pRet = m_deqConnectionInfo[m_nLoop++];
	else
	{
		m_nLoop = 0;
		pRet = m_deqConnectionInfo[m_nLoop++];
	}

	if(pRet->sock == INVALID_SOCKET)
		ConnectToTracker(pRet);
	return pRet;
}

ConnectionInfo* TrackerMgr::GetConnectionByAddr(ServerAddress *pTrackerAddr)
{
	UINT32					nFlag;
	struct sockaddr_in		s_in;
	char					cIP[16];
	int	nDeqSize	= (int)m_deqConnectionInfo.size();
	UINT32 nRet;
	for(int i = 0; i < nDeqSize; i++)
	{
		ConnectionInfo *pConnectionInfo = m_deqConnectionInfo[i];
		if(pConnectionInfo->port == pTrackerAddr->nPort &&
			0 == strcmp(pConnectionInfo->ip_addr, (const char*)pTrackerAddr->szIP))
		{
			//将被置为Invalid的Socket进行重连
			if(pConnectionInfo->sock == INVALID_SOCKET)
			{
				nRet = ConnectToTracker(pConnectionInfo);
				if(nRet != enumSuccess_FDFS)
				{
					return NULL;
				}
			}
			return pConnectionInfo;
		}
	}
	
	//没有找到，就创建一个新的ConnectionInfo
	ConnectionInfo *pNew = new ConnectionInfo;
	if(pNew == NULL)
	{
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("TrackerMgr::GetConnectionByAddr New Error"));
		return NULL;
	}
	memset(pNew, 0, sizeof(ConnectionInfo));
	pNew->sock = INVALID_SOCKET;

	pNew->port = pTrackerAddr->nPort;
	memcpy_s(pNew->ip_addr, sizeof(pNew->ip_addr), pTrackerAddr->szIP, sizeof(pTrackerAddr->szIP));
	InitializeCriticalSection(&pNew->csRecv);
	InitializeCriticalSection(&pNew->csSend);

	nRet = ConnectToTracker(pNew);
	if(nRet == enumSuccess_FDFS)
	{
		EnterCriticalSection(&m_csOperation);
		m_deqConnectionInfo.push_back(pNew);
		LeaveCriticalSection(&m_csOperation);
		return pNew;
	}
	else
	{
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("TrackerMgr::GetConnectionByAddr ConnectToTracker Failed Server IP:%s, Port:%d"), pTrackerAddr->szIP, pTrackerAddr->nPort);
		DeleteCriticalSection(&pNew->csRecv);
		DeleteCriticalSection(&pNew->csSend);
		delete pNew;
		return NULL;
	}
}

//recv不做重连
UINT32 TrackerMgr::QueryStorageStore(ConnectionInfo *pTrackerServer,
		ServerAddress *pStorageServer,
		TCHAR *szGroupName, UINT32 *nStorePathIndex)
{
	UINT32 nRet;
	UINT64 nInBytes;

	TrackerHeader header;
	memset(&header, 0, sizeof(header));
	header.byCmd = TRACKER_PROTO_CMD_SERVICE_QUERY_STORE_WITHOUT_GROUP_ONE;

	EnterCriticalSection(&pTrackerServer->csSend);
	if ((nRet = tcpsenddata_nb(pTrackerServer->sock, &header, sizeof(header), DEFAULT_NETWORK_TIMEOUT)) != 0)
	{
		closesocket(pTrackerServer->sock);
		pTrackerServer->sock = INVALID_SOCKET;
		LeaveCriticalSection(&pTrackerServer->csSend);
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("TrackerMgr::QueryStorageStore tcpsenddata_nb Header Failed, Error Code:%d"), nRet);
		return enumNetworkError_FDFS;
	}

	TrackerHeader resp;
	int nCount = 0;
	EnterCriticalSection(&pTrackerServer->csRecv);
	if ((nRet = tcprecvdata_nb(pTrackerServer->sock, &resp, sizeof(resp), DEFAULT_NETWORK_TIMEOUT, &nCount)) != 0)
	{
		closesocket(pTrackerServer->sock);
		pTrackerServer->sock = INVALID_SOCKET;
		LeaveCriticalSection(&pTrackerServer->csRecv);
		LeaveCriticalSection(&pTrackerServer->csSend);
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("TrackerMgr::QueryStorageStore tcprecvdata_nb Header Failed, Error Code:%d"), nRet);
		return enumNetworkError_FDFS;
	}

	if (resp.byStatus != 0)
	{
		closesocket(pTrackerServer->sock);
		pTrackerServer->sock = INVALID_SOCKET;
		LeaveCriticalSection(&pTrackerServer->csRecv);
		LeaveCriticalSection(&pTrackerServer->csSend);
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("TrackerMgr::QueryStorageStore Status Error, %d"), resp.byStatus);
		if(resp.byStatus == 28)
			return enumNoEnoughSpace_FDFS;
		return enumFailure_FDFS;
	}

	nInBytes = buff2long64((const char*)(resp.byPkgLen));
	if (nInBytes < 0)
	{
		closesocket(pTrackerServer->sock);
		pTrackerServer->sock = INVALID_SOCKET;
		LeaveCriticalSection(&pTrackerServer->csRecv);
		LeaveCriticalSection(&pTrackerServer->csSend);
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("TrackerMgr::QueryStorageStore Packet Len Error"));
		return enumFailure_FDFS;
	}

	//包头接收完毕，接收Body
	BYTE *pbyBuff = NULL;
	pbyBuff = new BYTE[nInBytes];
	if(pbyBuff == NULL)
	{
		closesocket(pTrackerServer->sock);
		pTrackerServer->sock = INVALID_SOCKET;
		LeaveCriticalSection(&pTrackerServer->csRecv);
		LeaveCriticalSection(&pTrackerServer->csSend);
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("TrackerMgr::QueryStorageStore New Error"));
		delete[] pbyBuff;
		return enumFailure_FDFS;
	}
	if ((nRet = tcprecvdata_nb(pTrackerServer->sock, pbyBuff, nInBytes, DEFAULT_NETWORK_TIMEOUT, &nCount)) != 0)
	{
		closesocket(pTrackerServer->sock);
		pTrackerServer->sock = INVALID_SOCKET;
		LeaveCriticalSection(&pTrackerServer->csRecv);
		LeaveCriticalSection(&pTrackerServer->csSend);
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("TrackerMgr::QueryStorageStore tcprecvdata_nb Body Failed, Error Code:%d"), nRet);
		delete[] pbyBuff;
		return enumNetworkError_FDFS;
	}
	LeaveCriticalSection(&pTrackerServer->csRecv);
	LeaveCriticalSection(&pTrackerServer->csSend);
	
	//数据接收完毕，开始解析
	memcpy(szGroupName, pbyBuff, FDFS_GROUP_NAME_MAX_LEN);
	*(szGroupName + FDFS_GROUP_NAME_MAX_LEN) = '\0';
	memcpy(pStorageServer->szIP, pbyBuff + FDFS_GROUP_NAME_MAX_LEN, IP_ADDRESS_SIZE - 1);
	pStorageServer->nPort = buff2long64((const char*)(pbyBuff + FDFS_GROUP_NAME_MAX_LEN + IP_ADDRESS_SIZE - 1));
	*nStorePathIndex = *(pbyBuff + FDFS_GROUP_NAME_MAX_LEN + IP_ADDRESS_SIZE - 1 + FDFS_PROTO_PKG_LEN_SIZE);

	delete[] pbyBuff;

	return enumSuccess_FDFS;
}

UINT32 TrackerMgr::QueryUpdateStorageStore(ConnectionInfo *pTrackerServer,
		const BYTE *pbyMasterGroupName, const BYTE *pbyMasterFileName,
		ServerAddress *pStorageServer,
		UINT32 *nStorePathIndex)
{
	UINT32 nRet;
	UINT64 nInBytes;

	BYTE byOutBuff[sizeof(TrackerHeader) + FDFS_GROUP_NAME_MAX_LEN
	+ FDFS_REMOTE_FILE_NAME_MAX_LEN];
	memset(byOutBuff, 0, sizeof(byOutBuff));

	TrackerHeader *pHeader = (TrackerHeader*)byOutBuff;
	memcpy(byOutBuff + sizeof(TrackerHeader), pbyMasterGroupName, FDFS_GROUP_NAME_MAX_LEN);

	UINT32 nMasterFileNameLen = strlen((const char*)pbyMasterFileName);
	memcpy(byOutBuff + sizeof(TrackerHeader) + FDFS_GROUP_NAME_MAX_LEN, pbyMasterFileName, nMasterFileNameLen);
	long642buff(FDFS_GROUP_NAME_MAX_LEN + nMasterFileNameLen, (char*)pHeader->byPkgLen);
	pHeader->byCmd = TRACKER_PROTO_CMD_SERVICE_QUERY_UPDATE;

	EnterCriticalSection(&pTrackerServer->csSend);
	if ((nRet = tcpsenddata_nb(pTrackerServer->sock, &byOutBuff, sizeof(TrackerHeader) + FDFS_GROUP_NAME_MAX_LEN + nMasterFileNameLen, DEFAULT_NETWORK_TIMEOUT)) != 0)
	{
		closesocket(pTrackerServer->sock);
		pTrackerServer->sock = INVALID_SOCKET;
		LeaveCriticalSection(&pTrackerServer->csSend);
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("TrackerMgr::QueryUpdateStorageStore tcpsenddata_nb Header Failed, Error Code:%d"), nRet);
		return enumNetworkError_FDFS;
	}

	TrackerHeader resp;
	int nCount = 0;
	EnterCriticalSection(&pTrackerServer->csRecv);
	if ((nRet = tcprecvdata_nb(pTrackerServer->sock, &resp, sizeof(resp), DEFAULT_NETWORK_TIMEOUT, &nCount)) != 0)
	{
		closesocket(pTrackerServer->sock);
		pTrackerServer->sock = INVALID_SOCKET;
		LeaveCriticalSection(&pTrackerServer->csRecv);
		LeaveCriticalSection(&pTrackerServer->csSend);
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("TrackerMgr::QueryUpdateStorageStore tcprecvdata_nb Header Failed, Error Code:%d"), nRet);
		return enumNetworkError_FDFS;
	}

	if (resp.byStatus != 0)
	{
		closesocket(pTrackerServer->sock);
		pTrackerServer->sock = INVALID_SOCKET;
		LeaveCriticalSection(&pTrackerServer->csRecv);
		LeaveCriticalSection(&pTrackerServer->csSend);
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("TrackerMgr::QueryUpdateStorageStore Status Error, %d"), resp.byStatus);
		return enumFailure_FDFS;
	}

	nInBytes = buff2long64((const char*)(resp.byPkgLen));
	if (nInBytes < 0)
	{
		closesocket(pTrackerServer->sock);
		pTrackerServer->sock = INVALID_SOCKET;
		LeaveCriticalSection(&pTrackerServer->csRecv);
		LeaveCriticalSection(&pTrackerServer->csSend);
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("TrackerMgr::QueryUpdateStorageStore Packet Len Error"));
		return enumFailure_FDFS;
	}

	//包头接收完毕，接收Body
	BYTE *pbyBuff = NULL;
	pbyBuff = new BYTE[nInBytes];
	if(pbyBuff == NULL)
	{
		closesocket(pTrackerServer->sock);
		pTrackerServer->sock = INVALID_SOCKET;
		LeaveCriticalSection(&pTrackerServer->csRecv);
		LeaveCriticalSection(&pTrackerServer->csSend);
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("TrackerMgr::QueryUpdateStorageStore New Error"));
		delete[] pbyBuff;
		return enumFailure_FDFS;
	}
	if ((nRet = tcprecvdata_nb(pTrackerServer->sock, pbyBuff, nInBytes, DEFAULT_NETWORK_TIMEOUT, &nCount)) != 0)
	{
		closesocket(pTrackerServer->sock);
		pTrackerServer->sock = INVALID_SOCKET;
		LeaveCriticalSection(&pTrackerServer->csRecv);
		LeaveCriticalSection(&pTrackerServer->csSend);
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("TrackerMgr::QueryUpdateStorageStore tcprecvdata_nb Body Failed, Error Code:%d"), nRet);
		delete[] pbyBuff;
		return enumNetworkError_FDFS;
	}
	LeaveCriticalSection(&pTrackerServer->csRecv);
	LeaveCriticalSection(&pTrackerServer->csSend);
	
	//数据接收完毕，开始解析
	memcpy(pStorageServer->szIP, pbyBuff + FDFS_GROUP_NAME_MAX_LEN, IP_ADDRESS_SIZE - 1);
	pStorageServer->nPort = buff2long64((const char*)(pbyBuff + FDFS_GROUP_NAME_MAX_LEN + IP_ADDRESS_SIZE - 1));

	delete[] pbyBuff;

	return enumSuccess_FDFS;
}

UINT32 TrackerMgr::QueryStorageFetch(ConnectionInfo *pTrackerServer,
	const BYTE *szGroupName, const BYTE *szRemoteFileName,
	ServerAddress *pStorageServer)
{
	UINT32 nRemoteFileNameLen;
	UINT32 nGroupNameLen;
	UINT32 nOutBytes = 0, nInBytes = 0;
	UINT32 nRet;
	BYTE *p = NULL;
	BYTE byOutBuff[sizeof(TrackerHeader) + FDFS_GROUP_NAME_MAX_LEN + FDFS_REMOTE_FILE_NAME_MAX_LEN];
	TrackerHeader *pHeader;

	memset(byOutBuff, 0, sizeof(byOutBuff));
	pHeader = (TrackerHeader *)byOutBuff;
	p = byOutBuff + sizeof(TrackerHeader);

	nGroupNameLen = strlen((const char*)szGroupName);
	memcpy_s(p, sizeof(byOutBuff) - (p - byOutBuff), szGroupName, nGroupNameLen);
	p += FDFS_GROUP_NAME_MAX_LEN;

	nRemoteFileNameLen = strlen((const char*)szRemoteFileName);
	memcpy_s(p, sizeof(byOutBuff) - (p - byOutBuff), szRemoteFileName, nRemoteFileNameLen);
	p += nRemoteFileNameLen;

	nOutBytes = p - byOutBuff;
	long642buff(nOutBytes - sizeof(TrackerHeader), (char*)pHeader->byPkgLen);
	pHeader->byCmd = TRACKER_PROTO_CMD_SERVICE_QUERY_FETCH_ONE;

	EnterCriticalSection(&pTrackerServer->csSend);
	if ((nRet = tcpsenddata_nb(pTrackerServer->sock, byOutBuff, nOutBytes, DEFAULT_NETWORK_TIMEOUT)) != 0)
	{
		closesocket(pTrackerServer->sock);
		pTrackerServer->sock = INVALID_SOCKET;
		LeaveCriticalSection(&pTrackerServer->csSend);
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("TrackerMgr::QueryStorageFetch tcpsenddata_nb Header Failed, Error Code:%d"), nRet);
		return enumNetworkError_FDFS;
	}

	//发包完毕，准备接收数据
	TrackerHeader resp;
	int nCount = 0;
	EnterCriticalSection(&pTrackerServer->csRecv);
	if ((nRet = tcprecvdata_nb(pTrackerServer->sock, &resp, sizeof(resp), DEFAULT_NETWORK_TIMEOUT, &nCount)) != 0)
	{
		closesocket(pTrackerServer->sock);
		pTrackerServer->sock = INVALID_SOCKET;
		LeaveCriticalSection(&pTrackerServer->csRecv);
		LeaveCriticalSection(&pTrackerServer->csSend);
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("TrackerMgr::QueryStorageFetch tcprecvdata_nb Header Failed, Error Code:%d"), nRet);
		return enumNetworkError_FDFS;
	}

	if (resp.byStatus != 0)
	{
		closesocket(pTrackerServer->sock);
		pTrackerServer->sock = INVALID_SOCKET;
		LeaveCriticalSection(&pTrackerServer->csRecv);
		LeaveCriticalSection(&pTrackerServer->csSend);
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("TrackerMgr::QueryStorageFetch Status Error, %d"), resp.byStatus);
		return enumFailure_FDFS;
	}

	nInBytes = buff2long64((const char*)(resp.byPkgLen));
	if (nInBytes < 0)
	{
		closesocket(pTrackerServer->sock);
		pTrackerServer->sock = INVALID_SOCKET;
		LeaveCriticalSection(&pTrackerServer->csRecv);
		LeaveCriticalSection(&pTrackerServer->csSend);
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("TrackerMgr::QueryStorageFetch Packet Len Error"));
		return enumFailure_FDFS;
	}

	//包头接收完毕，接收Body
	BYTE *pbyBuff = NULL;
	pbyBuff = new BYTE[nInBytes];
	if(pbyBuff == NULL)
	{
		closesocket(pTrackerServer->sock);
		pTrackerServer->sock = INVALID_SOCKET;
		LeaveCriticalSection(&pTrackerServer->csRecv);
		LeaveCriticalSection(&pTrackerServer->csSend);
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("TrackerMgr::QueryStorageFetch New Error"));
		delete[] pbyBuff;
		return enumFailure_FDFS;
	}
	if ((nRet = tcprecvdata_nb(pTrackerServer->sock, pbyBuff, nInBytes, DEFAULT_NETWORK_TIMEOUT, &nCount)) != 0)
	{
		closesocket(pTrackerServer->sock);
		pTrackerServer->sock = INVALID_SOCKET;
		LeaveCriticalSection(&pTrackerServer->csRecv);
		LeaveCriticalSection(&pTrackerServer->csSend);
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("TrackerMgr::QueryStorageFetch tcprecvdata_nb Body Failed, Error Code:%d"), nRet);
		delete[] pbyBuff;
		return enumNetworkError_FDFS;
	}
	LeaveCriticalSection(&pTrackerServer->csRecv);
	LeaveCriticalSection(&pTrackerServer->csSend);

	//数据接收完毕，开始解析
	//前16字节为Group Name，用不着
	//memcpy(szGroupName, pbyBuff, FDFS_GROUP_NAME_MAX_LEN);
	//*(szGroupName + FDFS_GROUP_NAME_MAX_LEN) = '\0';
	memcpy(pStorageServer->szIP, pbyBuff + FDFS_GROUP_NAME_MAX_LEN, IP_ADDRESS_SIZE - 1);
	pStorageServer->nPort = buff2long64((const char*)(pbyBuff + FDFS_GROUP_NAME_MAX_LEN + IP_ADDRESS_SIZE - 1));
	delete[] pbyBuff;
	return enumSuccess_FDFS;
}

UINT32 TrackerMgr::ConnectToTracker(ConnectionInfo *pTrackerServer)
{
	UINT32					nFlag;
	struct sockaddr_in		s_in;
	char					cIP[16];
	
	EnterCriticalSection(&pTrackerServer->csSend);
	if(pTrackerServer->sock != INVALID_SOCKET)
	{
		closesocket(pTrackerServer->sock);
		pTrackerServer->sock = INVALID_SOCKET;
	}

	pTrackerServer->sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(pTrackerServer->sock == INVALID_SOCKET)
	{
		LeaveCriticalSection(&pTrackerServer->csSend);
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("TrackerMgr::ConnectToTracker Socket Failed, %d"), WSAGetLastError());
		return enumFailure_FDFS;
	}

	_tcscpy_s(cIP, 16, pTrackerServer->ip_addr);
	s_in.sin_family			= AF_INET;
	s_in.sin_port			= htons((u_short)pTrackerServer->port);
	s_in.sin_addr.s_addr	= inet_addr(cIP);

	if(connect(pTrackerServer->sock, (struct sockaddr *)&s_in, sizeof(struct sockaddr)) == SOCKET_ERROR)
	{
		closesocket(pTrackerServer->sock);
		pTrackerServer->sock = INVALID_SOCKET;
		LeaveCriticalSection(&pTrackerServer->csSend);
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("TrackerMgr::ConnectToTracker Can't Connect Tracker Server IP:%s, Port:%d"), pTrackerServer->ip_addr, pTrackerServer->port);
		return enumNetworkError_FDFS;
	}

	nFlag = 1;
	if(ioctlsocket(pTrackerServer->sock, FIONBIO, (u_long*)&nFlag) == SOCKET_ERROR)   
	{
		closesocket(pTrackerServer->sock);
		pTrackerServer->sock = INVALID_SOCKET;
		LeaveCriticalSection(&pTrackerServer->csSend);
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("TrackerMgr::ConnectToTracker ioctlsocket Failed"));
		return enumFailure_FDFS;
	}
	if(setsockopt(pTrackerServer->sock, IPPROTO_TCP, TCP_NODELAY,  (char *)&nFlag, sizeof(nFlag)) != 0)
	{
		closesocket(pTrackerServer->sock);
		pTrackerServer->sock = INVALID_SOCKET;
		LeaveCriticalSection(&pTrackerServer->csSend);
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("TrackerMgr::ConnectToTracker setsockopt Failed"));
		return enumFailure_FDFS;
	}
	
	LeaveCriticalSection(&pTrackerServer->csSend);
	return enumSuccess_FDFS;
}

UINT32 TrackerMgr::GetGroupStat(ConnectionInfo *pTrackerServer,
	FDFSGroupStat *pStat, UINT32 nLen, UINT32 *pnStatCount)
{
	UINT32 nRet;
	UINT64 nInBytes;
	memset(pStat, 0, sizeof(FDFSGroupStat) * nLen);
	GroupStat rawStat[FDFS_MAX_GROUPS];

	TrackerHeader header;
	memset(&header, 0, sizeof(header));
	header.byCmd = TRACKER_PROTO_CMD_SERVER_LIST_ALL_GROUPS;

	EnterCriticalSection(&pTrackerServer->csSend);
	if ((nRet = tcpsenddata_nb(pTrackerServer->sock, &header, sizeof(header), DEFAULT_NETWORK_TIMEOUT)) != 0)
	{
		closesocket(pTrackerServer->sock);
		pTrackerServer->sock = INVALID_SOCKET;
		LeaveCriticalSection(&pTrackerServer->csSend);
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("TrackerMgr::GetGroupStat tcpsenddata_nb Header Failed, Error Code:%d"), nRet);
		return enumNetworkError_FDFS;
	}

	TrackerHeader resp;
	int nCount = 0;
	EnterCriticalSection(&pTrackerServer->csRecv);
	if ((nRet = tcprecvdata_nb(pTrackerServer->sock, &resp, sizeof(resp), DEFAULT_NETWORK_TIMEOUT, &nCount)) != 0)
	{
		closesocket(pTrackerServer->sock);
		pTrackerServer->sock = INVALID_SOCKET;
		LeaveCriticalSection(&pTrackerServer->csRecv);
		LeaveCriticalSection(&pTrackerServer->csSend);
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("TrackerMgr::GetGroupStat tcprecvdata_nb Header Failed, Error Code:%d"), nRet);
		return enumNetworkError_FDFS;
	}

	if (resp.byStatus != 0)
	{
		closesocket(pTrackerServer->sock);
		pTrackerServer->sock = INVALID_SOCKET;
		LeaveCriticalSection(&pTrackerServer->csRecv);
		LeaveCriticalSection(&pTrackerServer->csSend);
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("TrackerMgr::GetGroupStat Status Error, %d"), resp.byStatus);
		return enumFailure_FDFS;
	}

	nInBytes = buff2long64((const char*)(resp.byPkgLen));
	if (nInBytes < 0 || (nInBytes % sizeof(GroupStat)) != 0 || (nInBytes / sizeof(GroupStat)) > FDFS_MAX_GROUPS)
	{
		closesocket(pTrackerServer->sock);
		pTrackerServer->sock = INVALID_SOCKET;
		LeaveCriticalSection(&pTrackerServer->csRecv);
		LeaveCriticalSection(&pTrackerServer->csSend);
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("TrackerMgr::GetGroupStat Packet Len Error"));
		return enumFailure_FDFS;
	}
	*pnStatCount = nInBytes / sizeof(GroupStat);

	//包头接收完毕，接收Body
	if ((nRet = tcprecvdata_nb(pTrackerServer->sock, rawStat, nInBytes, DEFAULT_NETWORK_TIMEOUT, &nCount)) != 0)
	{
		closesocket(pTrackerServer->sock);
		pTrackerServer->sock = INVALID_SOCKET;
		LeaveCriticalSection(&pTrackerServer->csRecv);
		LeaveCriticalSection(&pTrackerServer->csSend);
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("TrackerMgr::GetGroupStat tcprecvdata_nb Body Failed, Error Code:%d"), nRet);
		return enumNetworkError_FDFS;
	}
	LeaveCriticalSection(&pTrackerServer->csRecv);
	LeaveCriticalSection(&pTrackerServer->csSend);

	//数据接收完毕，开始处理
	if(nLen < *pnStatCount)
	{
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("TrackerMgr::GetGroupStat Groups Is Too Many"));
		return enumFailure_FDFS;
	}
	for(int i = 0; i < *pnStatCount; i++)
	{
		memcpy(pStat[i].szGroupName, rawStat[i].szGroupName, FDFS_GROUP_NAME_MAX_LEN);
		pStat[i].nTotalMB = buff2long64((const char*)rawStat[i].byTotalMB);
		pStat[i].nFreeMB = buff2long64((const char*)rawStat[i].byFreeMB);
		pStat[i].nTrunkFreeMB = buff2long64((const char*)rawStat[i].byTrunkFreeMB);
		pStat[i].nCount = buff2long64((const char*)rawStat[i].byCount);
		pStat[i].nStoragePort = buff2long64((const char*)rawStat[i].byStoragePort);
		pStat[i].nStorageHttpPort = buff2long64((const char*)rawStat[i].byStorageHttpPort);
		pStat[i].nActiveCount = buff2long64((const char*)rawStat[i].byActiveCount);
		pStat[i].nCurrentWriteServer = buff2long64((const char*)rawStat[i].byCurrentWriteServer);
		pStat[i].nStorePathCount = buff2long64((const char*)rawStat[i].byStorePathCount);
		pStat[i].nSubdirCountPerPath = buff2long64((const char*)rawStat[i].bySubdirCountPerPath);
		pStat[i].nCurrentTrunkFileID = buff2long64((const char*)rawStat[i].byCurrentTrunkFileID);
	}
	return enumSuccess_FDFS;
}

UINT32 TrackerMgr::GetStorageStat(ConnectionInfo *pTrackerServer,
	TCHAR *pszGroupName, FDFSStorageStat *pStat, UINT32 nLen,
	UINT32 *pnStatCount)
{
	UINT32 nRet;
	UINT64 nInBytes;
	StorageStat rawStat[FDFS_MAX_GROUPS];
	BYTE byOut[sizeof(TrackerHeader) + FDFS_GROUP_NAME_MAX_LEN];
	memset(byOut, 0, sizeof(byOut));
	memset(pStat, 0, sizeof(FDFSStorageStat) * nLen);
	UINT32 nNameLen = strlen(pszGroupName);
	if (nNameLen > FDFS_GROUP_NAME_MAX_LEN)
	{
		nNameLen = FDFS_GROUP_NAME_MAX_LEN;
	}

	TrackerHeader *pHeader = (TrackerHeader*)byOut;
	pHeader->byCmd = TRACKER_PROTO_CMD_SERVER_LIST_STORAGE;
	memcpy(byOut + sizeof(TrackerHeader), pszGroupName, nNameLen);
	long642buff(FDFS_GROUP_NAME_MAX_LEN, (char*)pHeader->byPkgLen);

	EnterCriticalSection(&pTrackerServer->csSend);
	if ((nRet = tcpsenddata_nb(pTrackerServer->sock, byOut, sizeof(TrackerHeader) + FDFS_GROUP_NAME_MAX_LEN, DEFAULT_NETWORK_TIMEOUT)) != 0)
	{
		closesocket(pTrackerServer->sock);
		pTrackerServer->sock = INVALID_SOCKET;
		LeaveCriticalSection(&pTrackerServer->csSend);
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("TrackerMgr::GetStorageStat tcpsenddata_nb Header Failed, Error Code:%d"), nRet);
		return enumNetworkError_FDFS;
	}

	TrackerHeader resp;
	int nCount = 0;
	EnterCriticalSection(&pTrackerServer->csRecv);
	if ((nRet = tcprecvdata_nb(pTrackerServer->sock, &resp, sizeof(resp), DEFAULT_NETWORK_TIMEOUT, &nCount)) != 0)
	{
		closesocket(pTrackerServer->sock);
		pTrackerServer->sock = INVALID_SOCKET;
		LeaveCriticalSection(&pTrackerServer->csRecv);
		LeaveCriticalSection(&pTrackerServer->csSend);
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("TrackerMgr::GetStorageStat tcprecvdata_nb Header Failed, Error Code:%d"), nRet);
		return enumNetworkError_FDFS;
	}

	if (resp.byStatus != 0)
	{
		closesocket(pTrackerServer->sock);
		pTrackerServer->sock = INVALID_SOCKET;
		LeaveCriticalSection(&pTrackerServer->csRecv);
		LeaveCriticalSection(&pTrackerServer->csSend);
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("TrackerMgr::GetStorageStat Status Error, %d"), resp.byStatus);
		return enumFailure_FDFS;
	}

	nInBytes = buff2long64((const char*)(resp.byPkgLen));
	if (nInBytes < 0 || (nInBytes % sizeof(StorageStat)) != 0 || (nInBytes / sizeof(StorageStat)) > FDFS_MAX_SERVERS_EACH_GROUP)
	{
		closesocket(pTrackerServer->sock);
		pTrackerServer->sock = INVALID_SOCKET;
		LeaveCriticalSection(&pTrackerServer->csRecv);
		LeaveCriticalSection(&pTrackerServer->csSend);
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("TrackerMgr::GetStorageStat Packet Len Error"));
		return enumFailure_FDFS;
	}
	*pnStatCount = nInBytes / sizeof(StorageStat);

	//包头接收完毕，接收Body
	if ((nRet = tcprecvdata_nb(pTrackerServer->sock, rawStat, nInBytes, DEFAULT_NETWORK_TIMEOUT, &nCount)) != 0)
	{
		closesocket(pTrackerServer->sock);
		pTrackerServer->sock = INVALID_SOCKET;
		LeaveCriticalSection(&pTrackerServer->csRecv);
		LeaveCriticalSection(&pTrackerServer->csSend);
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("TrackerMgr::GetStorageStat tcprecvdata_nb Body Failed, Error Code:%d"), nRet);
		return enumNetworkError_FDFS;
	}
	LeaveCriticalSection(&pTrackerServer->csRecv);
	LeaveCriticalSection(&pTrackerServer->csSend);

	//数据接收完毕，开始处理
	if(nLen < *pnStatCount)
	{
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("TrackerMgr::GetStorageStat Storages Is Too Many"));
		return enumFailure_FDFS;
	}
	
	StorageStatBuff *pStatBuff = NULL;
	StorageStat *pSrc = NULL;
	FDFSStorageInfo *pStorageInfo = NULL;
	FDFSStorageStat *pDest = NULL;

	for(int i = 0; i < *pnStatCount; i++)
	{
		pDest = &pStat[i];
		pSrc = &rawStat[i];
		pStatBuff = &(pSrc->statBuff);
		pStorageInfo = &(pDest->stat);

		pDest->byStatus = pSrc->byStatus;
		//保留最后一个\0
		memcpy(pDest->szID, pSrc->byID, FDFS_STORAGE_ID_MAX_SIZE - 1);
		memcpy(pDest->szIpAddr, pSrc->byIP, IP_ADDRESS_SIZE - 1);
		memcpy(pDest->szSrcId, pSrc->bySrcID, FDFS_STORAGE_ID_MAX_SIZE - 1);
		strcpy(pDest->szDomainName, (const char*)pSrc->byDomainName);
		strcpy(pDest->szVersion, (const char*)pSrc->byVersion);
		pDest->joinTime = buff2long64((const char*)pSrc->byJoinTime);
		pDest->upTime = buff2long64((const char*)pSrc->byUpTime);
		pDest->nTotalMb = buff2long64((const char*)pSrc->byTotalMB);
		pDest->nFreeMb = buff2long64((const char*)pSrc->byFreeMB);
		pDest->nUploadPriority = buff2long64((const char*)pSrc->byUploadPriority);
		pDest->nStorePathCount = buff2long64((const char*)pSrc->byStorePathCount);
		pDest->nSubdirCountPerPath = buff2long64((const char*)pSrc->bySubdirCountPerPath);
		pDest->nStoragePort = buff2long64((const char*)pSrc->byStoragePort);
		pDest->nStorageHttpPort = buff2long64((const char*)pSrc->byStorageHttpPort);
		pDest->nCurrentWritePath = buff2long64((const char*)pSrc->byCurrentWritePath);

		pStorageInfo->connection.nAllocCount = buff2int((const char*)pStatBuff->connection.byAllocCount);
		pStorageInfo->connection.nCurrentCount = buff2int((const char*)pStatBuff->connection.byCurrentCount);
		pStorageInfo->connection.nMaxCount = buff2int((const char*)pStatBuff->connection.byMaxCount);

		pStorageInfo->nTotalUploadCount = buff2long64((const char*)pStatBuff->byTotalUploadCount);
		pStorageInfo->nSuccessUploadCount = buff2long64((const char*)pStatBuff->bySuccessUploadCount);
		pStorageInfo->nTotalAppendCount = buff2long64((const char*)pStatBuff->byTotalAppendCount);
		pStorageInfo->nSuccessAppendCount = buff2long64((const char*)pStatBuff->bySuccessAppendCount);
		pStorageInfo->nTotalModifyCount = buff2long64((const char*)pStatBuff->byTotalModifyCount);
		pStorageInfo->nSuccessModifyCount = buff2long64((const char*)pStatBuff->bySuccessModifyCount);
		pStorageInfo->nTotalTruncateCount = buff2long64((const char*)pStatBuff->byTotalTruncateCount);
		pStorageInfo->nSuccessTruncateCount = buff2long64((const char*)pStatBuff->bySuccessTruncateCount);
		pStorageInfo->nTotalSetMetaCount = buff2long64((const char*)pStatBuff->byTotalSetMetaCount);
		pStorageInfo->nSuccessSetMetaCount = buff2long64((const char*)pStatBuff->bySuccessSetMetaCount);

		pStorageInfo->nTotalDeleteCount = buff2long64((const char*)pStatBuff->byTotalDeleteCount);
		pStorageInfo->nSuccessDeleteCount = buff2long64((const char*)pStatBuff->bySuccessDeleteCount);
		pStorageInfo->nTotalDownloadCount = buff2long64((const char*)pStatBuff->byTotalDownloadCount);
		pStorageInfo->nSuccessDownloadCount = buff2long64((const char*)pStatBuff->bySuccessDownloadCount);
		pStorageInfo->nTotalGetMetaCount = buff2long64((const char*)pStatBuff->byTotalGetMetaCount);
		pStorageInfo->nSuccessGetMetaCount = buff2long64((const char*)pStatBuff->bySuccessGetMetaCount);
		pStorageInfo->lastSourceUpdate = buff2long64((const char*)pStatBuff->byLastSourceUpdate);
		pStorageInfo->lastSyncUpdate = buff2long64((const char*)pStatBuff->byLastSyncUpdate);
		pStorageInfo->lastSyncedTimestamp = buff2long64((const char*)pStatBuff->byLastSyncedTimestamp);
		pStorageInfo->nTotalCreateLinkCount = buff2long64((const char*)pStatBuff->byTotalCreateLinkCount);
		pStorageInfo->nSuccessCreateLinkCount = buff2long64((const char*)pStatBuff->bySuccessCreateLinkCount);
		pStorageInfo->nTotalDeleteLinkCount = buff2long64((const char*)pStatBuff->byTotalDeleteLinkCount);
		pStorageInfo->nSuccessDeleteLinkCount = buff2long64((const char*)pStatBuff->bySuccessDeleteLinkCount);
		pStorageInfo->nTotalUploadBytes = buff2long64((const char*)pStatBuff->byTotalUploadBytes);
		pStorageInfo->nSuccessUploadBytes = buff2long64((const char*)pStatBuff->bySuccessUploadBytes);
		pStorageInfo->nTotalAppendBytes = buff2long64((const char*)pStatBuff->byTotalAppendBytes);
		pStorageInfo->nSuccessAppendBytes = buff2long64((const char*)pStatBuff->bySuccessAppendBytes);
		pStorageInfo->nTotalModifyBytes = buff2long64((const char*)pStatBuff->byTotalModifyBytes);
		pStorageInfo->nSuccessModifyBytes = buff2long64((const char*)pStatBuff->bySuccessModifyBytes);
		pStorageInfo->nTotalDownloadBytes = buff2long64((const char*)pStatBuff->byTotalDownloadBytes);
		pStorageInfo->nSuccessDownloadBytes = buff2long64((const char*)pStatBuff->bySuccessDownloadBytes);
		pStorageInfo->nTotalSyncInBytes = buff2long64((const char*)pStatBuff->byTotalSyncInBytes);
		pStorageInfo->nSuccessSyncInBytes = buff2long64((const char*)pStatBuff->bySuccessSyncInBytes);
		pStorageInfo->nTotalSyncOutBytes = buff2long64((const char*)pStatBuff->byTotalSyncOutBytes);
		pStorageInfo->nSuccessSyncOutBytes = buff2long64((const char*)pStatBuff->bySuccessSyncOutBytes);
		pStorageInfo->nTotalFileOpenCount = buff2long64((const char*)pStatBuff->byTotalFileOpenCount);
		pStorageInfo->nSuccessFileOpenCount = buff2long64((const char*)pStatBuff->bySuccessFileOpenCount);
		pStorageInfo->nTotalFileReadCount = buff2long64((const char*)pStatBuff->byTotalFileReadCount);
		pStorageInfo->nSuccessFileReadCount = buff2long64((const char*)pStatBuff->bySuccessFileReadCount);
		pStorageInfo->nTotalFileWriteCount = buff2long64((const char*)pStatBuff->byTotalFileWriteCount);
		pStorageInfo->nSuccessFileWriteCount = buff2long64((const char*)pStatBuff->bySuccessFileWriteCount);
		pStorageInfo->lastHeartBeatTime = buff2long64((const char*)pStatBuff->byLastHeartBeatTime);
		pDest->bIfTrunkServer = pSrc->byIfTrunkServer;
	}
	return enumSuccess_FDFS;
}