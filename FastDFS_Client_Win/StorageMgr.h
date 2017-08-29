#ifndef STORAGE_MGR_H_
#define STORAGE_MGR_H_

#include "FastDFS_Client_Win.h"
#include "CommonDefine.h"
#include <deque>
using namespace std;

typedef deque<ConnectionInfo*>	ConnDeq;

class StorageMgr
{
public:
	StorageMgr();
	~StorageMgr();

	int UploadRawFile(
		ConnectionInfo *pStorageServer,
		int nStorePathIndex,
		const char *filename,
		const char *pbyFileExtName, 
		char *pbyRemoteFileName);
	int UploadBuffer(
		ConnectionInfo *pStorageServer,
		int nStorePathIndex,
		const char *pbyFileBuff,
		int nFileSize,
		const char *pbyFileExtName, 
		char *pbyRemoteFileName);
	UINT32 UploadFile(ServerAddress *pStorageServer,
		const TCHAR *szGroupName,
		UINT32 nStorePathIndex,
		const BYTE *pbyFileBuff,
		UINT32 nFileSize,
		const BYTE *pbyFileExtName, BYTE *pbyGroupName, BYTE *pbyRemoteFileName);
	UINT32 UploadSlaveFile(const ServerAddress *pAddr, const BYTE *pbyFileBuff, UINT32 nFileSize, const BYTE *pbyMasterFileName,
							const BYTE *pbyPrefixName, const BYTE *pbyFileExtName,
							BYTE *pbyGroupName, BYTE *pbyRemoteFileName);
	UINT32 DownloadFile(const ServerAddress *pAddr, const BYTE *pbyGroupName, const BYTE *pbyRemoteFileName, BYTE *pbyFileBuff, UINT32 *nFileSize);
	UINT32 DeleteFile(const ServerAddress *pAddr, const BYTE *pbyGroupName, const BYTE *pbyRemoteFileName);


	//连接存在就直接使用，不存在就通过参数创建
	ConnectionInfo* GetConnection(const ServerAddress *pStorageServer);
	UINT32 ConnectToStorage(ConnectionInfo *pStorageServer);

	CRITICAL_SECTION	m_csOperation;
	//CRITICAL_SECTION	m_csSend;
	//CRITICAL_SECTION	m_csRecv;
private:
	ConnDeq m_deqConnectionInfo;
};
#endif