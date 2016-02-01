#include <windows.h>
#include "StorageMgr.h"
#include "FastDFS_Client_Win.h"
#include "CommonDefine.h"
#include "FDFS_Packet.h"
#include "Sockopt.h"

StorageMgr::StorageMgr()
{
	InitializeCriticalSection(&m_csOperation);
}

StorageMgr::~StorageMgr()
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
	DeleteCriticalSection(&m_csOperation);
}

UINT32 StorageMgr::UploadFile(ServerAddress *pStorageAddr,
		const TCHAR *szGroupName,
		UINT32 nStorePathIndex,
		const BYTE *pbyFileBuff,
		UINT32 nFileSize,
		const BYTE *pbyFileExtName, BYTE *pbyGroupName, BYTE *pbyRemoteFileName)
{
	UINT32 nRet;
	BYTE byOutBuff[512];
	BYTE *p;
	ConnectionInfo *pStorageServer = NULL;
	pbyGroupName[0] = '\0';
	pbyRemoteFileName[0] = '\0';
	pStorageServer = GetConnection(pStorageAddr);
	if(pStorageServer == NULL)
	{
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("StorageMgr::UploadFile Connect Storage Failed IP:%s, Port:%d"), pStorageAddr->szIP, pStorageAddr->nPort);
		//只有在Tracker给的Storage无法访问时才返回NULL
		return enumNetworkError_FDFS;
	}

	TrackerHeader *pHeader = (TrackerHeader*)byOutBuff;
	p = byOutBuff + sizeof(TrackerHeader);
	*p++ = (BYTE)nStorePathIndex;

	long642buff(nFileSize, (char*)p);
	p += FDFS_PROTO_PKG_LEN_SIZE;

	memset(p, 0, FDFS_FILE_EXT_NAME_MAX_LEN);
	if (pbyFileExtName != NULL)
	{
		int nFileExtLen;

		nFileExtLen = strlen((const char*)pbyFileExtName);
		if (nFileExtLen > FDFS_FILE_EXT_NAME_MAX_LEN)
		{
			nFileExtLen = FDFS_FILE_EXT_NAME_MAX_LEN;
		}
		if (nFileExtLen > 0)
		{
			memcpy(p, pbyFileExtName, nFileExtLen);
		}
	}
	p += FDFS_FILE_EXT_NAME_MAX_LEN;

	long642buff((p - byOutBuff) + nFileSize - sizeof(TrackerHeader), (char*)pHeader->byPkgLen);
	pHeader->byCmd = STORAGE_PROTO_CMD_UPLOAD_FILE;
	pHeader->byStatus = 0;

	EnterCriticalSection(&pStorageServer->csSend);
	if ((nRet = tcpsenddata_nb(pStorageServer->sock, byOutBuff, p - byOutBuff, DEFAULT_NETWORK_TIMEOUT)) != 0)
	{
		closesocket(pStorageServer->sock);
		pStorageServer->sock = INVALID_SOCKET;
		LeaveCriticalSection(&pStorageServer->csSend);
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("StorageMgr::UploadFile tcpsenddata_nb Header Failed, Error Code:%d"), nRet);
		return enumNetworkError_FDFS;
	}

	//包头发送完毕，发送文件主体
	if ((nRet = tcpsenddata_nb(pStorageServer->sock, (char *)pbyFileBuff, nFileSize, DEFAULT_NETWORK_TIMEOUT)) != 0)
	{
		closesocket(pStorageServer->sock);
		pStorageServer->sock = INVALID_SOCKET;
		LeaveCriticalSection(&pStorageServer->csSend);
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("StorageMgr::UploadFile tcpsenddata_nb Body Failed, Error Code:%d"), nRet);
		return enumNetworkError_FDFS;
	}

	//文件发送完毕，接受服务器返回的文件id
	TrackerHeader resp;
	int nCount = 0;
	EnterCriticalSection(&pStorageServer->csRecv);
	if ((nRet = tcprecvdata_nb(pStorageServer->sock, &resp, sizeof(resp), DEFAULT_NETWORK_TIMEOUT, &nCount)) != 0)
	{
		closesocket(pStorageServer->sock);
		pStorageServer->sock = INVALID_SOCKET;
		LeaveCriticalSection(&pStorageServer->csRecv);
		LeaveCriticalSection(&pStorageServer->csSend);
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("StorageMgr::UploadFile tcprecvdata_nb ID Header Failed, Error Code:%d"), nRet);
		return enumNetworkError_FDFS;
	}

	if (resp.byStatus != 0)
	{
		closesocket(pStorageServer->sock);
		pStorageServer->sock = INVALID_SOCKET;
		LeaveCriticalSection(&pStorageServer->csRecv);
		LeaveCriticalSection(&pStorageServer->csSend);
		if(resp.byStatus == 28)
		{
			WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("StorageMgr::UploadFile FastDFS Cluster Don't Have Enough Space"), pbyGroupName, pbyRemoteFileName);
			return enumNoEnoughSpace_FDFS;
		}
		else
		{
			WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("StorageMgr::UploadFile Status Error, %d"), resp.byStatus);
			return enumFailure_FDFS;
		}
	}

	UINT32 nInBytes = buff2long64((const char*)(resp.byPkgLen));
	if (nInBytes < 0)
	{
		closesocket(pStorageServer->sock);
		pStorageServer->sock = INVALID_SOCKET;
		LeaveCriticalSection(&pStorageServer->csRecv);
		LeaveCriticalSection(&pStorageServer->csSend);
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("StorageMgr::UploadFile Packet Len Error"));
		return enumFailure_FDFS;
	}

	//包头接收完毕，接收Body
	BYTE *pbyInBuff = NULL;
	pbyInBuff = new BYTE[nInBytes];
	if(pbyInBuff == NULL)
	{
		closesocket(pStorageServer->sock);
		pStorageServer->sock = INVALID_SOCKET;
		LeaveCriticalSection(&pStorageServer->csRecv);
		LeaveCriticalSection(&pStorageServer->csSend);
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("StorageMgr::UploadFile New Error"));
		delete[] pbyInBuff;
		return enumFailure_FDFS;
	}
	if ((nRet = tcprecvdata_nb(pStorageServer->sock, pbyInBuff, nInBytes, DEFAULT_NETWORK_TIMEOUT, &nCount)) != 0)
	{
		closesocket(pStorageServer->sock);
		pStorageServer->sock = INVALID_SOCKET;
		LeaveCriticalSection(&pStorageServer->csRecv);
		LeaveCriticalSection(&pStorageServer->csSend);
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("StorageMgr::UploadFile tcprecvdata_nb ID Body Failed, Error Code:%d"), nRet);
		delete[] pbyInBuff;
		return enumNetworkError_FDFS;
	}
	LeaveCriticalSection(&pStorageServer->csRecv);
	LeaveCriticalSection(&pStorageServer->csSend);
	
	//数据接收完毕，开始解析
	memcpy(pbyGroupName, pbyInBuff, FDFS_GROUP_NAME_MAX_LEN);
	pbyGroupName[FDFS_GROUP_NAME_MAX_LEN] = '\0';

	memcpy(pbyRemoteFileName, pbyInBuff + FDFS_GROUP_NAME_MAX_LEN, \
		nInBytes - FDFS_GROUP_NAME_MAX_LEN);
	pbyRemoteFileName[nInBytes - FDFS_GROUP_NAME_MAX_LEN] = '\0';

	delete[] pbyInBuff;
	return enumSuccess_FDFS;
}

UINT32 StorageMgr::UploadSlaveFile(const ServerAddress *pStorageAddr, const BYTE *pbyFileBuff, UINT32 nFileSize, const BYTE *pbyMasterFileName,
							const BYTE *pbyPrefixName, const BYTE *pbyFileExtName,
							BYTE *pbyGroupName, BYTE *pbyRemoteFileName)
{
	UINT32 nRet;
	UINT32 nMasterFileNameLen, nPrefixNameLen;
	BYTE byOutBuff[512];
	BYTE *p;
	ConnectionInfo *pStorageServer = NULL;
	pbyGroupName[0] = '\0';
	pbyRemoteFileName[0] = '\0';
	pStorageServer = GetConnection(pStorageAddr);
	if(pStorageServer == NULL)
	{
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("StorageMgr::UploadFile Connect Storage Failed Server IP:%s, Port:%d"), pStorageAddr->szIP, pStorageAddr->nPort);
		//只有在Tracker给的Storage无法访问时才返回NULL
		return enumNetworkError_FDFS;
	}

	nMasterFileNameLen = strlen((const char*)pbyMasterFileName);
	nPrefixNameLen = strlen((const char*)pbyPrefixName);

	TrackerHeader *pHeader = (TrackerHeader*)byOutBuff;
	p = byOutBuff + sizeof(TrackerHeader);

	long642buff(nMasterFileNameLen, (char*)p);
	p += FDFS_PROTO_PKG_LEN_SIZE;

	long642buff(nFileSize, (char*)p);
	p += FDFS_PROTO_PKG_LEN_SIZE;

	memset(p, 0, FDFS_FILE_PREFIX_MAX_LEN + FDFS_FILE_EXT_NAME_MAX_LEN);
	
	if(nPrefixNameLen > FDFS_FILE_PREFIX_MAX_LEN)
	{
		nPrefixNameLen = FDFS_FILE_PREFIX_MAX_LEN;
	}
	memcpy_s(p, FDFS_FILE_PREFIX_MAX_LEN, pbyPrefixName, nPrefixNameLen);
	p += FDFS_FILE_PREFIX_MAX_LEN;

	if (pbyFileExtName != NULL)
	{
		int nFileExtLen;

		nFileExtLen = strlen((const char*)pbyFileExtName);
		if (nFileExtLen > FDFS_FILE_EXT_NAME_MAX_LEN)
		{
			nFileExtLen = FDFS_FILE_EXT_NAME_MAX_LEN;
		}
		if (nFileExtLen > 0)
		{
			memcpy(p, pbyFileExtName, nFileExtLen);
		}
	}
	p += FDFS_FILE_EXT_NAME_MAX_LEN;

	memcpy_s(p, FDFS_REMOTE_FILE_NAME_MAX_LEN, pbyMasterFileName, nMasterFileNameLen);
	p += nMasterFileNameLen;

	long642buff((p - byOutBuff) + nFileSize - sizeof(TrackerHeader), (char*)pHeader->byPkgLen);
	pHeader->byCmd = STORAGE_PROTO_CMD_UPLOAD_SLAVE_FILE;
	pHeader->byStatus = 0;

	EnterCriticalSection(&pStorageServer->csSend);
	if ((nRet = tcpsenddata_nb(pStorageServer->sock, byOutBuff, p - byOutBuff, DEFAULT_NETWORK_TIMEOUT)) != 0)
	{
		closesocket(pStorageServer->sock);
		pStorageServer->sock = INVALID_SOCKET;
		LeaveCriticalSection(&pStorageServer->csSend);
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("StorageMgr::UploadSlaveFile tcpsenddata_nb Header Failed, Error Code:%d"), nRet);
		return enumNetworkError_FDFS;
	}

	//包头发送完毕，发送文件主体
	if ((nRet = tcpsenddata_nb(pStorageServer->sock, (char *)pbyFileBuff, nFileSize, DEFAULT_NETWORK_TIMEOUT)) != 0)
	{
		closesocket(pStorageServer->sock);
		pStorageServer->sock = INVALID_SOCKET;
		LeaveCriticalSection(&pStorageServer->csSend);
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("StorageMgr::UploadSlaveFile tcpsenddata_nb Body Failed, Error Code:%d"), nRet);
		return enumNetworkError_FDFS;
	}

	//文件发送完毕，接受服务器返回的文件id
	TrackerHeader resp;
	int nCount = 0;
	EnterCriticalSection(&pStorageServer->csRecv);
	if ((nRet = tcprecvdata_nb(pStorageServer->sock, &resp, sizeof(resp), DEFAULT_NETWORK_TIMEOUT, &nCount)) != 0)
	{
		closesocket(pStorageServer->sock);
		pStorageServer->sock = INVALID_SOCKET;
		LeaveCriticalSection(&pStorageServer->csRecv);
		LeaveCriticalSection(&pStorageServer->csSend);
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("StorageMgr::UploadSlaveFile tcprecvdata_nb ID Header Failed, Error Code:%d"), nRet);
		return enumNetworkError_FDFS;
	}

	if (resp.byStatus != 0)
	{		
		closesocket(pStorageServer->sock);
		pStorageServer->sock = INVALID_SOCKET;
		LeaveCriticalSection(&pStorageServer->csRecv);
		LeaveCriticalSection(&pStorageServer->csSend);
		if(resp.byStatus == 28)
		{
			WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("StorageMgr::UploadSlaveFile FastDFS Cluster Don't Have Enough Space"), pbyGroupName, pbyRemoteFileName);
			return enumNoEnoughSpace_FDFS;
		}
		else
		{
			WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("StorageMgr::UploadSlaveFile Status Error, %d"), resp.byStatus);
			return enumFailure_FDFS;
		}
	}

	UINT32 nInBytes = buff2long64((const char*)(resp.byPkgLen));
	if (nInBytes < 0)
	{
		closesocket(pStorageServer->sock);
		pStorageServer->sock = INVALID_SOCKET;
		LeaveCriticalSection(&pStorageServer->csRecv);
		LeaveCriticalSection(&pStorageServer->csSend);
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("StorageMgr::UploadSlaveFile Packet Len Error"));
		return enumFailure_FDFS;
	}

	//包头接收完毕，接收Body
	BYTE *pbyInBuff = NULL;
	pbyInBuff = new BYTE[nInBytes];
	if(pbyInBuff == NULL)
	{
		closesocket(pStorageServer->sock);
		pStorageServer->sock = INVALID_SOCKET;
		LeaveCriticalSection(&pStorageServer->csRecv);
		LeaveCriticalSection(&pStorageServer->csSend);
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("StorageMgr::UploadSlaveFile New Error"));
		delete[] pbyInBuff;
		return enumFailure_FDFS;
	}
	if ((nRet = tcprecvdata_nb(pStorageServer->sock, pbyInBuff, nInBytes, DEFAULT_NETWORK_TIMEOUT, &nCount)) != 0)
	{
		closesocket(pStorageServer->sock);
		pStorageServer->sock = INVALID_SOCKET;
		LeaveCriticalSection(&pStorageServer->csRecv);
		LeaveCriticalSection(&pStorageServer->csSend);
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("StorageMgr::UploadSlaveFile tcprecvdata_nb ID Body Failed, Error Code:%d"), nRet);
		delete[] pbyInBuff;
		return enumNetworkError_FDFS;
	}
	LeaveCriticalSection(&pStorageServer->csRecv);
	LeaveCriticalSection(&pStorageServer->csSend);
	
	//数据接收完毕，开始解析
	memcpy(pbyGroupName, pbyInBuff, FDFS_GROUP_NAME_MAX_LEN);
	pbyGroupName[FDFS_GROUP_NAME_MAX_LEN] = '\0';

	memcpy(pbyRemoteFileName, pbyInBuff + FDFS_GROUP_NAME_MAX_LEN, \
		nInBytes - FDFS_GROUP_NAME_MAX_LEN);
	pbyRemoteFileName[nInBytes - FDFS_GROUP_NAME_MAX_LEN] = '\0';

	delete[] pbyInBuff;
	return enumSuccess_FDFS;
}

UINT32 StorageMgr::DownloadFile(const ServerAddress *pStorageAddr,
								const BYTE *pbyGroupName,
								const BYTE *pbyRemoteFileName,
								BYTE *pbyFileBuff, UINT32 *nFileSize)
{
	UINT32 nRet;
	UINT32 nRemoteFileNameLen;
	BYTE byOutBuff[512];
	BYTE *p;
	ConnectionInfo *pStorageServer = NULL;
	pStorageServer = GetConnection(pStorageAddr);
	if(pStorageServer == NULL)
	{
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("StorageMgr::DownloadFile Connect Storage Failed"));
		//只有在Tracker给的Storage无法访问时才返回NULL
		return enumNetworkError_FDFS;
	}

	TrackerHeader *pHeader = (TrackerHeader*)byOutBuff;
	p = byOutBuff + sizeof(TrackerHeader);

	long642buff(0, (char*)p);
	p += FDFS_PROTO_PKG_LEN_SIZE;

	long642buff(0, (char*)p);
	p += FDFS_PROTO_PKG_LEN_SIZE;

	memcpy(p, pbyGroupName, FDFS_GROUP_NAME_MAX_LEN);
	p += FDFS_GROUP_NAME_MAX_LEN;

	nRemoteFileNameLen = strlen((const char*)pbyRemoteFileName);
	memcpy(p, pbyRemoteFileName, nRemoteFileNameLen);
	p += nRemoteFileNameLen;

	long642buff((p - byOutBuff) - sizeof(TrackerHeader), (char*)pHeader->byPkgLen);
	pHeader->byCmd = STORAGE_PROTO_CMD_DOWNLOAD_FILE;
	pHeader->byStatus = 0;

	EnterCriticalSection(&pStorageServer->csSend);
	if ((nRet = tcpsenddata_nb(pStorageServer->sock, byOutBuff, p - byOutBuff, DEFAULT_NETWORK_TIMEOUT)) != 0)
	{
		closesocket(pStorageServer->sock);
		pStorageServer->sock = INVALID_SOCKET;
		LeaveCriticalSection(&pStorageServer->csSend);
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("StorageMgr::DownloadFile tcpsenddata_nb Header Failed, Error Code:%d"), nRet);
		return enumNetworkError_FDFS;
	}

	//请求发送完毕，准备接收数据
	TrackerHeader resp;
	int nCount = 0;
	EnterCriticalSection(&pStorageServer->csRecv);
	if ((nRet = tcprecvdata_nb(pStorageServer->sock, &resp, sizeof(resp), DEFAULT_NETWORK_TIMEOUT, &nCount)) != 0)
	{
		closesocket(pStorageServer->sock);
		pStorageServer->sock = INVALID_SOCKET;
		LeaveCriticalSection(&pStorageServer->csRecv);
		LeaveCriticalSection(&pStorageServer->csSend);
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("StorageMgr::DownloadFile tcprecvdata_nb Header Failed, Error Code:%d"), nRet);
		return enumNetworkError_FDFS;
	}

	//TODO(xxx):先Leave临界区，然后再做其他事情
	if (resp.byStatus != 0)
	{
		closesocket(pStorageServer->sock);
		pStorageServer->sock = INVALID_SOCKET;
		LeaveCriticalSection(&pStorageServer->csRecv);
		LeaveCriticalSection(&pStorageServer->csSend);
		if(resp.byStatus == 2)
		{
			WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("StorageMgr::DownloadFile Failed File id:%s/%s Not Exists"), pbyGroupName, pbyRemoteFileName);
			return enumFileNotExists_FDFS;
		}
		else
		{
			WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("StorageMgr::DownloadFile Status Error, %d"), resp.byStatus);
			return enumFailure_FDFS;
		}
	}

	UINT32 nInBytes = buff2long64((const char*)(resp.byPkgLen));
	if (nInBytes < 0)
	{
		closesocket(pStorageServer->sock);
		pStorageServer->sock = INVALID_SOCKET;
		LeaveCriticalSection(&pStorageServer->csRecv);
		LeaveCriticalSection(&pStorageServer->csSend);
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("StorageMgr::DownloadFile Packet Len Error"));
		return enumFailure_FDFS;
	}

	//包头接收完毕，接收Body
	if ((nRet = tcprecvdata_nb(pStorageServer->sock, pbyFileBuff, nInBytes, DEFAULT_NETWORK_TIMEOUT, &nCount)) != 0)
	{
		closesocket(pStorageServer->sock);
		pStorageServer->sock = INVALID_SOCKET;
		LeaveCriticalSection(&pStorageServer->csRecv);
		LeaveCriticalSection(&pStorageServer->csSend);
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("StorageMgr::DownloadFile tcprecvdata_nb Body Failed, Error Code:%d"), nRet);
		return enumNetworkError_FDFS;
	}
	LeaveCriticalSection(&pStorageServer->csRecv);
	LeaveCriticalSection(&pStorageServer->csSend);
	
	*nFileSize = nInBytes;

	return enumSuccess_FDFS;
}

//pStorageServer应该包含ip和端口
UINT32 StorageMgr::ConnectToStorage(ConnectionInfo *pStorageServer)
{
	UINT32					nFlag;
	struct sockaddr_in		s_in;
	char					cIP[16];

	EnterCriticalSection(&pStorageServer->csSend);
	if(pStorageServer->sock != INVALID_SOCKET)
	{
		closesocket(pStorageServer->sock);
		pStorageServer->sock = INVALID_SOCKET;
	}

	pStorageServer->sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(pStorageServer->sock == INVALID_SOCKET)
	{
		LeaveCriticalSection(&pStorageServer->csSend);
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("StorageMgr::ConnectToStorage socket Failed, %d"), WSAGetLastError());
		return enumFailure_FDFS;
	}

	_tcscpy_s(cIP, 16, pStorageServer->ip_addr);
	s_in.sin_family			= AF_INET;
	s_in.sin_port			= htons((u_short)pStorageServer->port);
	s_in.sin_addr.s_addr	= inet_addr(cIP);

	if(connect(pStorageServer->sock, (struct sockaddr *)&s_in, sizeof(struct sockaddr)) == SOCKET_ERROR)
	{
		closesocket(pStorageServer->sock);
		pStorageServer->sock = INVALID_SOCKET;
		LeaveCriticalSection(&pStorageServer->csSend);
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("StorageMgr::ConnectToStorage connect Failed, Error Code:%d"), WSAGetLastError());
		return enumNetworkError_FDFS;
	}

	nFlag = 1;
	if(ioctlsocket(pStorageServer->sock, FIONBIO, (u_long*)&nFlag) == SOCKET_ERROR)   
	{
		closesocket(pStorageServer->sock);
		pStorageServer->sock = INVALID_SOCKET;
		LeaveCriticalSection(&pStorageServer->csSend);
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("StorageMgr::ConnectToStorage ioctlsocket Failed, Error Code:%d"), WSAGetLastError());
		return enumFailure_FDFS;
	}
	if(setsockopt(pStorageServer->sock, IPPROTO_TCP, TCP_NODELAY,  (char *)&nFlag, sizeof(nFlag)) != 0)
	{
		closesocket(pStorageServer->sock);
		pStorageServer->sock = INVALID_SOCKET;
		LeaveCriticalSection(&pStorageServer->csSend);
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("StorageMgr::ConnectToStorage setsockopt Failed, Error Code:%d"), WSAGetLastError());
		return enumFailure_FDFS;
	}

	LeaveCriticalSection(&pStorageServer->csSend);
	return enumSuccess_FDFS;
}

ConnectionInfo* StorageMgr::GetConnection(const ServerAddress *pStorageServer)
{
	UINT32					nFlag;
	struct sockaddr_in		s_in;
	char					cIP[16];
	int	nDeqSize	= (int)m_deqConnectionInfo.size();
	UINT32 nRet;
	for(int i = 0; i < nDeqSize; i++)
	{
		ConnectionInfo *pConnectionInfo = m_deqConnectionInfo[i];
		if(pConnectionInfo->port == pStorageServer->nPort &&
			0 == strcmp(pConnectionInfo->ip_addr, (const char*)pStorageServer->szIP))
		{
			//将被置为Invalid的Socket进行重连
			if(pConnectionInfo->sock == INVALID_SOCKET)
			{
				nRet = ConnectToStorage(pConnectionInfo);
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
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("StorageMgr::GetConnection New Error"));
		return NULL;
	}
	memset(pNew, 0, sizeof(ConnectionInfo));
	pNew->sock = INVALID_SOCKET;

	pNew->port = pStorageServer->nPort;
	memcpy_s(pNew->ip_addr, sizeof(pNew->ip_addr), pStorageServer->szIP, sizeof(pStorageServer->szIP));
	InitializeCriticalSection(&pNew->csRecv);
	InitializeCriticalSection(&pNew->csSend);

	nRet = ConnectToStorage(pNew);
	if(nRet == enumSuccess_FDFS)
	{
		EnterCriticalSection(&m_csOperation);
		m_deqConnectionInfo.push_back(pNew);
		LeaveCriticalSection(&m_csOperation);
		return pNew;
	}
	else
	{
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("StorageMgr::GetConnection ConnectToStorage Failed Server IP:%s, Port:%d"), pStorageServer->szIP, pStorageServer->nPort);
		DeleteCriticalSection(&pNew->csRecv);
		DeleteCriticalSection(&pNew->csSend);
		delete pNew;
		return NULL;
	}
}

UINT32 StorageMgr::DeleteFile(const ServerAddress *pStorageAddr, const BYTE *pbyGroupName, const BYTE *pbyRemoteFileName)
{
	UINT32 nRemoteFileNameLen;
	UINT32 nGroupNameLen;
	UINT32 nOutBytes = 0, nInBytes = 0;
	UINT32 nRet;
	BYTE *p = NULL;
	BYTE byOutBuff[sizeof(TrackerHeader) + FDFS_GROUP_NAME_MAX_LEN + FDFS_REMOTE_FILE_NAME_MAX_LEN];
	TrackerHeader *pHeader;

	ConnectionInfo *pStorageServer = NULL;
	pStorageServer = GetConnection(pStorageAddr);
	if(pStorageServer == NULL)
	{
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("StorageMgr::DeleteFile Connect Storage Failed Server IP:%s, Port:%d"), pStorageAddr->szIP, pStorageAddr->nPort);
		return enumFailure_FDFS;
	}

	memset(byOutBuff, 0, sizeof(byOutBuff));
	pHeader = (TrackerHeader *)byOutBuff;
	p = byOutBuff + sizeof(TrackerHeader);

	nGroupNameLen = strlen((const char*)pbyGroupName);
	memcpy_s(p, sizeof(byOutBuff) - (p - byOutBuff), pbyGroupName, nGroupNameLen);
	p += FDFS_GROUP_NAME_MAX_LEN;

	nRemoteFileNameLen = strlen((const char*)pbyRemoteFileName);
	memcpy_s(p, sizeof(byOutBuff) - (p - byOutBuff), pbyRemoteFileName, nRemoteFileNameLen);
	p += nRemoteFileNameLen;

	nOutBytes = p - byOutBuff;
	long642buff(nOutBytes - sizeof(TrackerHeader), (char*)pHeader->byPkgLen);
	pHeader->byCmd = STORAGE_PROTO_CMD_DELETE_FILE;

	EnterCriticalSection(&pStorageServer->csSend);
	if ((nRet = tcpsenddata_nb(pStorageServer->sock, byOutBuff, nOutBytes, DEFAULT_NETWORK_TIMEOUT)) != 0)
	{
		closesocket(pStorageServer->sock);
		pStorageServer->sock = INVALID_SOCKET;
		LeaveCriticalSection(&pStorageServer->csSend);
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("StorageMgr::DeleteFile tcpsenddata_nb Header Failed, Error Code:%d"), nRet);
		return enumNetworkError_FDFS;
	}

	//发包完毕，准备接收响应
	TrackerHeader resp;
	int nCount = 0;
	EnterCriticalSection(&pStorageServer->csRecv);
	if ((nRet = tcprecvdata_nb(pStorageServer->sock, &resp, sizeof(resp), DEFAULT_NETWORK_TIMEOUT, &nCount)) != 0)
	{
		closesocket(pStorageServer->sock);
		pStorageServer->sock = INVALID_SOCKET;
		LeaveCriticalSection(&pStorageServer->csRecv);
		LeaveCriticalSection(&pStorageServer->csSend);
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("StorageMgr::DeleteFile tcprecvdata_nb Header Failed, Error Code:%d"), nRet);
		return enumNetworkError_FDFS;
	}
	LeaveCriticalSection(&pStorageServer->csRecv);
	LeaveCriticalSection(&pStorageServer->csSend);

	if (resp.byStatus != 0)
	{
		if(resp.byStatus == 2)
		{
			WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("StorageMgr::DeleteFile Failed, File id:%s/%s Not Exists"), pbyGroupName, pbyRemoteFileName);
			return enumFileNotExists_FDFS;
		}
		else
		{
			WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("StorageMgr::DeleteFile Status Error, %d"), resp.byStatus);
			return enumFailure_FDFS;
		}
	}

	nInBytes = buff2long64((const char*)(resp.byPkgLen));
	if (nInBytes < 0)
	{
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("StorageMgr::DeleteFile Packet Len Error"));
		return enumFailure_FDFS;
	}

	if(resp.byCmd != TRACKER_PROTO_CMD_RESP)
	{
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("StorageMgr::DeleteFile Not Expected Resp Packet"));
		return enumFailure_FDFS;
	}
	return enumSuccess_FDFS;
}