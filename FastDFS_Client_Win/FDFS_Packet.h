#ifndef FDFS_PACKET_H_
#define FDFS_PACKET_H_

#include "CommonDefine.h"

#define TRACKER_PROTO_CMD_SERVER_LIST_ALL_GROUPS		91
#define TRACKER_PROTO_CMD_SERVER_LIST_STORAGE			92
#define TRACKER_PROTO_CMD_RESP					100
#define TRACKER_PROTO_CMD_SERVICE_QUERY_STORE_WITHOUT_GROUP_ONE	101
#define TRACKER_PROTO_CMD_SERVICE_QUERY_FETCH_ONE		102
#define TRACKER_PROTO_CMD_SERVICE_QUERY_UPDATE  		103
#define STORAGE_PROTO_CMD_UPLOAD_FILE		11
#define STORAGE_PROTO_CMD_DELETE_FILE		12
#define STORAGE_PROTO_CMD_DOWNLOAD_FILE		14
#define STORAGE_PROTO_CMD_UPLOAD_SLAVE_FILE	21

typedef struct
{
	BYTE byPkgLen[FDFS_PROTO_PKG_LEN_SIZE];
	BYTE byCmd;
	BYTE byStatus;
} TrackerHeader;

typedef struct
{
	TCHAR	szGroupName[FDFS_GROUP_NAME_MAX_LEN + 1];
	BYTE	byTotalMB[FDFS_PROTO_PKG_LEN_SIZE]; //total disk storage in MB
	BYTE	byFreeMB[FDFS_PROTO_PKG_LEN_SIZE];  //free disk storage in MB
	BYTE	byTrunkFreeMB[FDFS_PROTO_PKG_LEN_SIZE];  //trunk free space in MB
	BYTE	byCount[FDFS_PROTO_PKG_LEN_SIZE];    //server count
	BYTE	byStoragePort[FDFS_PROTO_PKG_LEN_SIZE];
	BYTE	byStorageHttpPort[FDFS_PROTO_PKG_LEN_SIZE];
	BYTE	byActiveCount[FDFS_PROTO_PKG_LEN_SIZE]; //active server count
	BYTE	byCurrentWriteServer[FDFS_PROTO_PKG_LEN_SIZE];
	BYTE	byStorePathCount[FDFS_PROTO_PKG_LEN_SIZE];
	BYTE	bySubdirCountPerPath[FDFS_PROTO_PKG_LEN_SIZE];
	BYTE	byCurrentTrunkFileID[FDFS_PROTO_PKG_LEN_SIZE];
} GroupStat;

/* struct for network transfering */
typedef struct
{
    struct {
        BYTE byAllocCount[4];
        BYTE byCurrentCount[4];
        BYTE byMaxCount[4];
    } connection;

	BYTE byTotalUploadCount[8];
	BYTE bySuccessUploadCount[8];
	BYTE byTotalAppendCount[8];
	BYTE bySuccessAppendCount[8];
	BYTE byTotalModifyCount[8];
	BYTE bySuccessModifyCount[8];
	BYTE byTotalTruncateCount[8];
	BYTE bySuccessTruncateCount[8];
	BYTE byTotalSetMetaCount[8];
	BYTE bySuccessSetMetaCount[8];
	BYTE byTotalDeleteCount[8];
	BYTE bySuccessDeleteCount[8];
	BYTE byTotalDownloadCount[8];
	BYTE bySuccessDownloadCount[8];
	BYTE byTotalGetMetaCount[8];
	BYTE bySuccessGetMetaCount[8];
	BYTE byTotalCreateLinkCount[8];
	BYTE bySuccessCreateLinkCount[8];
	BYTE byTotalDeleteLinkCount[8];
	BYTE bySuccessDeleteLinkCount[8];
	BYTE byTotalUploadBytes[8];
	BYTE bySuccessUploadBytes[8];
	BYTE byTotalAppendBytes[8];
	BYTE bySuccessAppendBytes[8];
	BYTE byTotalModifyBytes[8];
	BYTE bySuccessModifyBytes[8];
	BYTE byTotalDownloadBytes[8];
	BYTE bySuccessDownloadBytes[8];
	BYTE byTotalSyncInBytes[8];
	BYTE bySuccessSyncInBytes[8];
	BYTE byTotalSyncOutBytes[8];
	BYTE bySuccessSyncOutBytes[8];
	BYTE byTotalFileOpenCount[8];
	BYTE bySuccessFileOpenCount[8];
	BYTE byTotalFileReadCount[8];
	BYTE bySuccessFileReadCount[8];
	BYTE byTotalFileWriteCount[8];
	BYTE bySuccessFileWriteCount[8];
	BYTE byLastSourceUpdate[8];
	BYTE byLastSyncUpdate[8];
	BYTE byLastSyncedTimestamp[8];
	BYTE byLastHeartBeatTime[8];
} StorageStatBuff;

typedef struct
{
	BYTE byStatus;
	BYTE byID[FDFS_STORAGE_ID_MAX_SIZE];
	BYTE byIP[IP_ADDRESS_SIZE];
	BYTE byDomainName[FDFS_DOMAIN_NAME_MAX_SIZE];
	BYTE bySrcID[FDFS_STORAGE_ID_MAX_SIZE];  //src storage id
	BYTE byVersion[FDFS_VERSION_SIZE];
	BYTE byJoinTime[8];
	BYTE byUpTime[8];
	BYTE byTotalMB[8];
	BYTE byFreeMB[8];
	BYTE byUploadPriority[8];
	BYTE byStorePathCount[8];
	BYTE bySubdirCountPerPath[8];
	BYTE byCurrentWritePath[8];
	BYTE byStoragePort[8];
	BYTE byStorageHttpPort[8];
	StorageStatBuff statBuff;
	BYTE byIfTrunkServer;
} StorageStat;

#endif