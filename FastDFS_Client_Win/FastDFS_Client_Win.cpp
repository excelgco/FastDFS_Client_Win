#include <windows.h>
#include "FastDFS_Client_Win.h"
#include "CommonDefine.h"
#include "TrackerMgr.h"
#include "StorageMgr.h"

int g_nLogLevel = 1;

TrackerMgr *pTrackerMgr;
StorageMgr *pStorageMgr;

UINT32 __stdcall FDFSC_Initialize(ServerAddress *pAddr,
								 UINT32 nAddrCount,
								 UINT32 nLogLevel)
{
	g_nLogLevel = nLogLevel;

	UINT32 nRet;
	WSADATA wsd;
	//socket库初始化
	nRet = WSAStartup(MAKEWORD(2,2), &wsd);
	if(nRet != 0)
	{
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("FDFSC_Initialize WSAStartup Failed"));
		return enumFailure_FDFS;
	}

	pTrackerMgr = new TrackerMgr();
	pStorageMgr = new StorageMgr();
	if(pTrackerMgr == NULL || pStorageMgr == NULL)
	{
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("FDFSC_Initialize Failed"));
		return enumFailure_FDFS;
	}

	nRet = pTrackerMgr->Initialize(pAddr, nAddrCount);
	if(nRet != enumSuccess_FDFS)
	{
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("FDFSC_Initialize Failed"));
	}
	else
	{
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("FDFSC_Initialize Succeed"));
	}
	return nRet;
}

void __stdcall FDFSC_UnInitialize()
{
	pTrackerMgr->UnInitialize();
	if(pTrackerMgr)
	{
		delete pTrackerMgr;
		pTrackerMgr = NULL;
	}
	if(pStorageMgr)
	{
		delete pStorageMgr;
		pStorageMgr = NULL;
	}
	
	WSACleanup();
	return;
}

UINT32 __stdcall FDFSC_UploadFile(const BYTE *pbyFileBuff, UINT32 nFileSize, const TCHAR *pszFileExtName,
	TCHAR *pszGroupName, TCHAR *pszRemoteFileName)
{
	UINT32 nRet;
	if(pbyFileBuff == NULL || nFileSize <= 0 || pszFileExtName == NULL || pszGroupName == NULL || pszRemoteFileName == NULL)
	{
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("FDFSC_UploadFile Invalid Parameters"));
		return enumInvalidParameters_FDFS;
	}

	ConnectionInfo *pTrackerServer = NULL;
	pTrackerServer = pTrackerMgr->GetConnection();
	if(pTrackerServer == NULL)
	{
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("FDFSC_UploadFile Can't Get A Tracker Server"));
		return enumNetworkError_FDFS;
	}

	ServerAddress storageAddress;
	UINT32 nStorePathIndex = 0;
	memset(&storageAddress, 0, sizeof(ServerAddress));
	TCHAR szGroupName[FDFS_GROUP_NAME_MAX_LEN + 1];
	nRet = pTrackerMgr->QueryStorageStore(pTrackerServer, &storageAddress, szGroupName, &nStorePathIndex);
	if(nRet != enumSuccess_FDFS)
	{
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("FDFSC_UploadFile Can't Get A Storage Server"));
		return nRet;
	}

	nRet = pStorageMgr->UploadFile(&storageAddress, szGroupName, nStorePathIndex, pbyFileBuff,
		nFileSize, (const BYTE*)pszFileExtName, (BYTE*)pszGroupName, (BYTE*)pszRemoteFileName);
	if(nRet != enumSuccess_FDFS)
	{
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("FDFSC_UploadFile Upload Failed"));
	}
	else
	{
		WriteLogInfo(LogFileName, FDFSC_DEBUG_MODE, _T("FDFSC_UploadFile Succeed,File id: %s/%s"), pszGroupName, pszRemoteFileName);
	}
	return nRet;
}

UINT32 __stdcall FDFSC_UploadFileByID(const BYTE *pbyFileBuff, UINT32 nFileSize, const TCHAR *pszFileExtName,
	TCHAR *pszFileID)
{
	TCHAR byGroupName[FDFS_GROUP_NAME_MAX_LEN + 1];
	TCHAR byRemoteFileName[FDFS_REMOTE_FILE_NAME_MAX_LEN + 1];
	UINT32 nRet =  FDFSC_UploadFile(pbyFileBuff, nFileSize, pszFileExtName, byGroupName, byRemoteFileName);
	if(nRet == enumSuccess_FDFS)
		sprintf_s(pszFileID, FDFS_GROUP_NAME_MAX_LEN + FDFS_REMOTE_FILE_NAME_MAX_LEN + 1, "%s/%s", byGroupName, byRemoteFileName);
	else
		pszFileID[0] = '\0';
	return nRet;
}

UINT32 __stdcall FDFSC_UploadSlaveFile(const BYTE *pbyFileBuff, UINT32 nFileSize,
	const TCHAR *pszMasterGroupName, const TCHAR *pszMasterFileName,
	const TCHAR *pszPrefixName, const TCHAR *pszFileExtName,
	TCHAR *pszGroupName, TCHAR *pszRemoteFileName)
{
	UINT32 nRet;
	if(pbyFileBuff == NULL || nFileSize <= 0 || pszMasterGroupName == NULL || pszMasterFileName == NULL
		|| pszPrefixName == NULL || pszFileExtName == NULL
		|| pszGroupName == NULL || pszRemoteFileName == NULL)
	{
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("FDFSC_UploadSlaveFile Invalid Parameters"));
		return enumInvalidParameters_FDFS;
	}

	ConnectionInfo *pTrackerServer = NULL;
	pTrackerServer = pTrackerMgr->GetConnection();
	if(pTrackerServer == NULL)
	{
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("FDFSC_UploadSlaveFile Can't Get A Tracker Server"));
		return enumNetworkError_FDFS;
	}

	ServerAddress storageAddress;
	UINT32 nStorePathIndex = 0;
	memset(&storageAddress, 0, sizeof(ServerAddress));
	TCHAR szGroupName[FDFS_GROUP_NAME_MAX_LEN + 1];
	nRet = pTrackerMgr->QueryUpdateStorageStore(pTrackerServer, (const BYTE*)pszMasterGroupName, (const BYTE*)pszMasterFileName, &storageAddress, &nStorePathIndex);
	if(nRet != enumSuccess_FDFS)
	{
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("FDFSC_UploadSlaveFile Can't Get A Storage Server"));
		return nRet;
	}

	nRet = pStorageMgr->UploadSlaveFile(&storageAddress, pbyFileBuff, nFileSize, (const BYTE*)pszMasterFileName,
		(const BYTE*)pszPrefixName, (const BYTE*)pszFileExtName, (BYTE*)pszGroupName, (BYTE*)pszRemoteFileName);
	if(nRet != enumSuccess_FDFS)
	{
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("FDFSC_UploadSlaveFile Upload Failed"));
	}
	else
	{
		WriteLogInfo(LogFileName, FDFSC_DEBUG_MODE, _T("FDFSC_UploadSlaveFile Succeed,File id: %s/%s"), pszGroupName, pszRemoteFileName);
	}
	return nRet;
}

UINT32 __stdcall FDFSC_UploadSlaveFileByID(const BYTE *pbyFileBuff, UINT32 nFileSize,
	const TCHAR *pszMasterGroupName, const TCHAR *pszMasterFileName,
	const TCHAR *pszPrefixName, const TCHAR *pszFileExtName,
	TCHAR *pszFileID)
{
	TCHAR szGroupName[FDFS_GROUP_NAME_MAX_LEN + 1];
	TCHAR szRemoteFileName[FDFS_REMOTE_FILE_NAME_MAX_LEN + 1];
	UINT32 nRet =  FDFSC_UploadSlaveFile(pbyFileBuff, nFileSize, pszMasterGroupName, pszMasterFileName, pszPrefixName, pszFileExtName, szGroupName, szRemoteFileName);
	if(nRet == enumSuccess_FDFS)
		sprintf_s((char*)pszFileID, FDFS_GROUP_NAME_MAX_LEN + FDFS_REMOTE_FILE_NAME_MAX_LEN + 1, "%s/%s", szGroupName, szRemoteFileName);
	else
		pszFileID[0] = '\0';
	return nRet;
}

UINT32 __stdcall FDFSC_DownloadFile(const TCHAR *pszGroupName, const TCHAR *pszRemoteFileName,
		BYTE *pbyFileBuff, UINT32 *nFileSize)
{
	UINT32 nRet;
	if(pbyFileBuff == NULL || nFileSize <= 0 || pszGroupName == NULL || pszRemoteFileName == NULL)
	{
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("FDFSC_DownloadFile Invalid Parameters"));
		return enumInvalidParameters_FDFS;
	}

	ConnectionInfo *pTrackerServer = NULL;
	pTrackerServer = pTrackerMgr->GetConnection();
	if(pTrackerServer == NULL)
	{
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("FDFSC_DownloadFile Can't Get A Tracker Server"));
		return enumNetworkError_FDFS;
	}

	ServerAddress storageAddress;
	memset(&storageAddress, 0, sizeof(ServerAddress));
	nRet = pTrackerMgr->QueryStorageFetch(pTrackerServer, (const BYTE*)pszGroupName, (const BYTE*)pszRemoteFileName, &storageAddress);
	if(nRet != enumSuccess_FDFS)
	{
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("FDFSC_DownloadFile Can't Get A Storage Server"));
		return nRet;
	}

	nRet = pStorageMgr->DownloadFile(&storageAddress, (const BYTE*)pszGroupName, (const BYTE*)pszRemoteFileName, pbyFileBuff, nFileSize);
	if(nRet != enumSuccess_FDFS)
	{
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("FDFSC_DownloadFile Download Failed"));
	}
	else
	{
		WriteLogInfo(LogFileName, FDFSC_DEBUG_MODE, _T("FDFSC_DownloadFile Succeed,File id: %s/%s"), pszGroupName, pszRemoteFileName);
	}
	return nRet;
}

UINT32 __stdcall FDFSC_DownloadFileByID(const TCHAR *pszFileID, BYTE *pbyFileBuff, UINT32 *nFileSize)
{
	//拆ID
	TCHAR szGroupName[FDFS_GROUP_NAME_MAX_LEN + 1];
	TCHAR szRemoteFileName[FDFS_REMOTE_FILE_NAME_MAX_LEN + 1];
	UINT32 nIDLen = 0;
	UINT32 nRemoteFileNameLen = 0;

	int i;

	for(i = 0; i < FDFS_GROUP_NAME_MAX_LEN; i++)
	{
		if((char)pszFileID[i] == '/')
			break;
	}
	if(i == FDFS_GROUP_NAME_MAX_LEN)
	{
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("FDFSC_DownloadFileByID Malformed File ID:%s"), pszFileID);
		return enumInvalidParameters_FDFS;
	}

	nIDLen = _tcslen(pszFileID);
	nRemoteFileNameLen = nIDLen - i - 1;
	memcpy_s(szGroupName, sizeof(szGroupName), pszFileID, i);
	szGroupName[i] = '\0';
	memcpy_s(szRemoteFileName, sizeof(szRemoteFileName), pszFileID + i + 1, nRemoteFileNameLen);
	szRemoteFileName[nRemoteFileNameLen] = '\0';
	
	return FDFSC_DownloadFile(szGroupName, szRemoteFileName, pbyFileBuff, nFileSize);
}

UINT32 __stdcall FDFSC_DownloadFileEx(const TCHAR *pszTrackerIPList, const TCHAR *pszGroupName, const TCHAR *pszRemoteFileName,
		BYTE *pbyFileBuff, UINT32 *nFileSize)
{
	UINT32 nRet;
	if(pszTrackerIPList == NULL || pbyFileBuff == NULL || nFileSize <= 0 || pszGroupName == NULL || pszRemoteFileName == NULL)
	{
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("FDFSC_DownloadFileEx Invalid Parameters"));
		return enumInvalidParameters_FDFS;
	}

	ConnectionInfo *pTrackerServer = NULL;
	ServerAddress addr;
	const TCHAR *pszBegin = pszTrackerIPList;
	const TCHAR *pszEnd = pszTrackerIPList;
	UINT32 nTrackerCount = 0;
	while(1)
	{
		if(*pszEnd == ';' || *pszEnd == '\0')
		{
			UINT32 nLen = pszEnd - pszBegin;
			if(nLen > 0)
			{
				strncpy(addr.szIP, pszBegin, nLen);
				addr.szIP[nLen] = '\0';
				addr.nPort = 22122;
				pTrackerServer = pTrackerMgr->GetConnectionByAddr(&addr);
				if(pTrackerServer != NULL)
				{
					break;
				}
			}
			if(*pszEnd == '\0')
				break;
			pszEnd++;
			pszBegin = pszEnd;
		}
		else
		{
			pszEnd++;
		}
	}
	if(pTrackerServer == NULL)
	{
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("FDFSC_DeleteFileEx Can't Get A Tracker Server"));
		return enumNetworkError_FDFS;
	}

	ServerAddress storageAddress;
	memset(&storageAddress, 0, sizeof(ServerAddress));
	nRet = pTrackerMgr->QueryStorageFetch(pTrackerServer, (const BYTE*)pszGroupName, (const BYTE*)pszRemoteFileName, &storageAddress);
	if(nRet != enumSuccess_FDFS)
	{
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("FDFSC_DownloadFileEx Can't Get A Storage Server"));
		return nRet;
	}

	nRet = pStorageMgr->DownloadFile(&storageAddress, (const BYTE*)pszGroupName, (const BYTE*)pszRemoteFileName, pbyFileBuff, nFileSize);
	if(nRet != enumSuccess_FDFS)
	{
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("FDFSC_DownloadFileEx Download Failed"));
	}
	else
	{
		WriteLogInfo(LogFileName, FDFSC_DEBUG_MODE, _T("FDFSC_DownloadFileEx Succeed,File id: %s/%s"), pszGroupName, pszRemoteFileName);
	}
	return nRet;
}

UINT32 __stdcall FDFSC_DownloadFileByIDEx(const TCHAR *pszTrackerIPList, const TCHAR *pszFileID,
		BYTE *pbyFileBuff, UINT32 *nFileSize)
{
	//拆ID
	TCHAR szGroupName[FDFS_GROUP_NAME_MAX_LEN + 1];
	TCHAR szRemoteFileName[FDFS_REMOTE_FILE_NAME_MAX_LEN + 1];
	UINT32 nIDLen = 0;
	UINT32 nRemoteFileNameLen = 0;

	int i;

	for(i = 0; i < FDFS_GROUP_NAME_MAX_LEN; i++)
	{
		if((char)pszFileID[i] == '/')
			break;
	}
	if(i == FDFS_GROUP_NAME_MAX_LEN)
	{
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("FDFSC_DownloadFileByID Malformed File ID:%s"), pszFileID);
		return enumInvalidParameters_FDFS;
	}

	nIDLen = _tcslen(pszFileID);
	nRemoteFileNameLen = nIDLen - i - 1;
	memcpy_s(szGroupName, sizeof(szGroupName), pszFileID, i);
	szGroupName[i] = '\0';
	memcpy_s(szRemoteFileName, sizeof(szRemoteFileName), pszFileID + i + 1, nRemoteFileNameLen);
	szRemoteFileName[nRemoteFileNameLen] = '\0';
	
	return FDFSC_DownloadFileEx(pszTrackerIPList, szGroupName, szRemoteFileName, pbyFileBuff, nFileSize);
}

UINT32 __stdcall FDFSC_DeleteFile(const TCHAR *pszGroupName, const TCHAR *pszRemoteFileName)
{
	UINT32 nRet;
	if(pszGroupName == NULL || pszRemoteFileName == NULL)
	{
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("FDFSC_DeleteFile Invalid Parameters"));
		return enumInvalidParameters_FDFS;
	}

	ConnectionInfo *pTrackerServer = NULL;
	pTrackerServer = pTrackerMgr->GetConnection();
	if(pTrackerServer == NULL)
	{
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("FDFSC_DeleteFile Can't Get A Tracker Server"));
		return enumNetworkError_FDFS;
	}

	ServerAddress storageAddress;
	memset(&storageAddress, 0, sizeof(ServerAddress));
	nRet = pTrackerMgr->QueryStorageFetch(pTrackerServer, (const BYTE*)pszGroupName, (const BYTE*)pszRemoteFileName, &storageAddress);
	if(nRet != enumSuccess_FDFS)
	{
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("FDFSC_DeleteFile Can't Get A Storage Server"));
		return nRet;
	}

	nRet = pStorageMgr->DeleteFile(&storageAddress, (const BYTE*)pszGroupName, (const BYTE*)pszRemoteFileName);
	if(nRet != enumSuccess_FDFS)
	{
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("FDFSC_DeleteFile DeleteFile Failed"));
	}
	else
	{
		WriteLogInfo(LogFileName, FDFSC_DEBUG_MODE, _T("FDFSC_DeleteFile Succeed,File id: %s/%s"), pszGroupName, pszRemoteFileName);
	}
	return nRet;
}

UINT32 __stdcall FDFSC_DeleteFileByID(const TCHAR *pszFileID)
{
	//拆ID
	TCHAR szGroupName[FDFS_GROUP_NAME_MAX_LEN + 1];
	TCHAR szRemoteFileName[FDFS_REMOTE_FILE_NAME_MAX_LEN + 1];
	UINT32 nIDLen = 0;
	UINT32 nRemoteFileNameLen = 0;

	int i;

	for(i = 0; i < FDFS_GROUP_NAME_MAX_LEN; i++)
	{
		if(pszFileID[i] == '/')
			break;
	}
	if(i == FDFS_GROUP_NAME_MAX_LEN)
	{
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("FDFSC_DownloadFileByID Malformed File ID:%s"), pszFileID);
		return enumInvalidParameters_FDFS;
	}

	nIDLen = _tcslen(pszFileID);
	nRemoteFileNameLen = nIDLen - i - 1;
	memcpy_s(szGroupName, sizeof(szGroupName), pszFileID, i);
	szGroupName[i] = '\0';
	memcpy_s(szRemoteFileName, sizeof(szRemoteFileName), pszFileID + i + 1, nRemoteFileNameLen);
	szRemoteFileName[nRemoteFileNameLen] = '\0';
	
	return FDFSC_DeleteFile(szGroupName, szRemoteFileName);
}

UINT32 __stdcall FDFSC_DeleteFileEx(const TCHAR *pszTrackerIPList, const TCHAR *pszGroupName, const TCHAR *pszRemoteFileName)
{
	UINT32 nRet;
	if(pszGroupName == NULL || pszRemoteFileName == NULL)
	{
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("FDFSC_DeleteFileEx Invalid Parameters"));
		return enumInvalidParameters_FDFS;
	}

	ConnectionInfo *pTrackerServer = NULL;
	ServerAddress addr;
	const TCHAR *pszBegin = pszTrackerIPList;
	const TCHAR *pszEnd = pszTrackerIPList;
	UINT32 nTrackerCount = 0;
	while(1)
	{
		if(*pszEnd == ';' || *pszEnd == '\0')
		{
			UINT32 nLen = pszEnd - pszBegin;
			if(nLen > 0)
			{
				strncpy(addr.szIP, pszBegin, nLen);
				addr.szIP[nLen] = '\0';
				addr.nPort = 22122;
				pTrackerServer = pTrackerMgr->GetConnectionByAddr(&addr);
				if(pTrackerServer != NULL)
				{
					break;
				}
			}
			if(*pszEnd == '\0')
				break;
			pszEnd++;
			pszBegin = pszEnd;
		}
		else
		{
			pszEnd++;
		}
	}
	if(pTrackerServer == NULL)
	{
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("FDFSC_DeleteFileEx Can't Get A Tracker Server"));
		return enumNetworkError_FDFS;
	}

	ServerAddress storageAddress;
	memset(&storageAddress, 0, sizeof(ServerAddress));
	nRet = pTrackerMgr->QueryStorageFetch(pTrackerServer, (const BYTE*)pszGroupName, (const BYTE*)pszRemoteFileName, &storageAddress);
	if(nRet != enumSuccess_FDFS)
	{
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("FDFSC_DeleteFileEx Can't Get A Storage Server"));
		return nRet;
	}

	nRet = pStorageMgr->DeleteFile(&storageAddress, (const BYTE*)pszGroupName, (const BYTE*)pszRemoteFileName);
	if(nRet != enumSuccess_FDFS)
	{
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("FDFSC_DeleteFileEx DeleteFile Failed"));
	}
	else
	{
		WriteLogInfo(LogFileName, FDFSC_DEBUG_MODE, _T("FDFSC_DeleteFileEx Succeed,File id: %s/%s"), pszGroupName, pszRemoteFileName);
	}
	return nRet;
}

UINT32 __stdcall FDFSC_DeleteFileByIDEx(const TCHAR *pszTrackerIPList, const TCHAR *pszFileID)
{
	TCHAR szGroupName[FDFS_GROUP_NAME_MAX_LEN + 1];
	TCHAR szRemoteFileName[FDFS_REMOTE_FILE_NAME_MAX_LEN + 1];
	UINT32 nIDLen = 0;
	UINT32 nRemoteFileNameLen = 0;

	int i;

	for(i = 0; i < FDFS_GROUP_NAME_MAX_LEN; i++)
	{
		if(pszFileID[i] == '/')
			break;
	}
	if(i == FDFS_GROUP_NAME_MAX_LEN)
	{
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("FDFSC_DownloadFileByID Malformed File ID:%s"), pszFileID);
		return enumInvalidParameters_FDFS;
	}

	nIDLen = _tcslen(pszFileID);
	nRemoteFileNameLen = nIDLen - i - 1;
	memcpy_s(szGroupName, sizeof(szGroupName), pszFileID, i);
	szGroupName[i] = '\0';
	memcpy_s(szRemoteFileName, sizeof(szRemoteFileName), pszFileID + i + 1, nRemoteFileNameLen);
	szRemoteFileName[nRemoteFileNameLen] = '\0';
	
	return FDFSC_DeleteFileEx(pszTrackerIPList, szGroupName, szRemoteFileName);
}

UINT32 __stdcall FDFSC_TrackerListGroups(ServerAddress *pTrackerAddr, FDFSGroupStat *pStat, UINT32 nLen, UINT32 *pnStatCount)
{
	if(pTrackerAddr == NULL || pStat == NULL || nLen > FDFS_MAX_GROUPS || pnStatCount == NULL)
	{
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("FDFSC_TrackerListGroups Invalid Parameters"));
		return enumInvalidParameters_FDFS;
	}

	ConnectionInfo *pTrackerServer = NULL;
	pTrackerServer = pTrackerMgr->GetConnectionByAddr(pTrackerAddr);
	if(pTrackerServer == NULL)
	{
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("FDFSC_TrackerListGroups Can't Find Tracker Server %s:%d"), pTrackerAddr->szIP, pTrackerAddr->nPort);
		return enumInvalidParameters_FDFS;
	}

	return pTrackerMgr->GetGroupStat(pTrackerServer, pStat, nLen, pnStatCount);
}

UINT32 __stdcall FDFSC_TrackerListStorages(ServerAddress *pTrackerAddr, TCHAR *pszGroupName, FDFSStorageStat *pStat, UINT32 nLen, UINT32 *pnStatCount)
{
	if(pTrackerAddr == NULL || pszGroupName == NULL || 0 == strcmp(_T(""), pszGroupName) || pStat == NULL || nLen > FDFS_MAX_SERVERS_EACH_GROUP || pnStatCount == NULL)
	{
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("FDFSC_TrackerListStorages Invalid Parameters"));
		return enumInvalidParameters_FDFS;
	}
	
	ConnectionInfo *pTrackerServer = NULL;
	pTrackerServer = pTrackerMgr->GetConnectionByAddr(pTrackerAddr);
	if(pTrackerServer == NULL)
	{
		WriteLogInfo(LogFileName, FDFSC_ERROR_MODE, _T("FDFSC_TrackerListStorages Can't Find Tracker Server %s:%d"), pTrackerAddr->szIP, pTrackerAddr->nPort);
		return enumInvalidParameters_FDFS;
	}

	return pTrackerMgr->GetStorageStat(pTrackerServer, pszGroupName, pStat, nLen, pnStatCount);
}

UINT32 __stdcall FDFSC_CheckConfiguration(const TCHAR *pszTrackerIPList)
{
	UINT32 nRet;
	BOOL bFirst = FALSE;
	FDFSGroupStat standardGroupStat[FDFS_MAX_GROUPS];
	UINT32 nStandardGroupCount = 0;
	ServerAddress standardAddr;

	FDFSGroupStat tempGroupStat[FDFS_MAX_GROUPS];
	UINT32 nTempGroupCount = 0;
	ServerAddress tempAddr;

	const TCHAR *pszBegin = pszTrackerIPList;
	const TCHAR *pszEnd = pszTrackerIPList;
	UINT32 nTrackerCount = 0;
	while(1)
	{
		if(*pszEnd == ';' || *pszEnd == '\0')
		{
			UINT32 nLen = pszEnd - pszBegin;
			if(nLen > 0)
			{
				if(bFirst == FALSE)
				{
					//将第一个Tracker作为Standard
					strncpy(standardAddr.szIP, pszBegin, nLen);
					standardAddr.szIP[nLen] = '\0';
					standardAddr.nPort = 22122;
					nRet = FDFSC_TrackerListGroups(&standardAddr, standardGroupStat, FDFS_MAX_GROUPS, &nStandardGroupCount);
					if(nRet != enumSuccess_FDFS)
						return enumFailure_FDFS;
					bFirst = TRUE;
				}
				else
				{
					strncpy(tempAddr.szIP, pszBegin, nLen);
					tempAddr.szIP[nLen] = '\0';
					tempAddr.nPort = 22122;
					nRet = FDFSC_TrackerListGroups(&tempAddr, tempGroupStat, FDFS_MAX_GROUPS, &nTempGroupCount);
					if(nRet != enumSuccess_FDFS)
						return enumFailure_FDFS;
					//比较temp和standard的配置
					if(nStandardGroupCount != nTempGroupCount)
						return enumFailure_FDFS;
					for(UINT32 i = 0; i < nStandardGroupCount; i++)
					{
						UINT32 j;
						for(j = 0; j < nStandardGroupCount; j++)
						{
							if(0 == _tcscmp(standardGroupStat[i].szGroupName, tempGroupStat[j].szGroupName))
							{
								//同一个Group的判断内部的服务器数量是否一致
								if(standardGroupStat[i].nCount == tempGroupStat[j].nCount)
									break;
								else
									return enumFailure_FDFS;
							}
						}
						if(j == nStandardGroupCount)
							return enumFailure_FDFS;
					}
				}
			}
			if(*pszEnd == '\0')
				break;
			pszEnd++;
			pszBegin = pszEnd;
		}
		else
		{
			pszEnd++;
		}
	}

	return enumSuccess_FDFS;
}