#ifndef TRACKER_MGR_H_
#define TRACKER_MGR_H_

#include "FastDFS_Client_Win.h"
#include "CommonDefine.h"
#include <deque>
using namespace std;

typedef deque<ConnectionInfo*>	ConnDeq;

class TrackerMgr
{
public:
	TrackerMgr();
	~TrackerMgr();

	UINT32 Initialize(ServerAddress *pAddr, UINT32 nArrCount);
	void UnInitialize();

	ConnectionInfo* GetConnectionByAddr(ServerAddress *pTrackerAddr);
	ConnectionInfo* GetConnection();
	UINT32 QueryStorageStore(ConnectionInfo *pTrackerServer,
		ServerAddress *pStorageServer,
		TCHAR *szGroupName, UINT32 *nStorePathIndex);
	//用于从文件上传查询
	UINT32 QueryUpdateStorageStore(ConnectionInfo *pTrackerServer,
		const BYTE *szMasterGroupName, const BYTE *szMasterFileName,
		ServerAddress *pStorageServer,
		UINT32 *nStorePathIndex);
	UINT32 QueryStorageFetch(ConnectionInfo *pTrackerServer,
		const BYTE *szGroupName, const BYTE *szRemoteFileName,
		ServerAddress *pStorageServer);
	//查询Group状态信息
	UINT32 GetGroupStat(ConnectionInfo *pTrackerServer,
		FDFSGroupStat *pStat, UINT32 nLen, UINT32 *pnStatCount);
	//查询Storage状态信息
	UINT32 GetStorageStat(ConnectionInfo *pTrackerServer,
		TCHAR *pszGroupName, FDFSStorageStat *pStat, UINT32 nLen,
		UINT32 *pnStatCount);
private:
	CRITICAL_SECTION	m_csOperation;
	//CRITICAL_SECTION	m_csSend;
	//CRITICAL_SECTION	m_csRecv;
	//不采用Deque
	UINT32 m_nLoop;
	ConnDeq m_deqConnectionInfo;
	
	UINT32 ConnectToTracker(ConnectionInfo *pTrackerServer);

};
#endif