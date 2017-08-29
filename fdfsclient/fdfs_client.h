#ifndef FDFSCLIENT_H_
#define FDFSCLIENT_H_
#include <stdint.h>
#include "CommonDefine.h"
#include "FastDFS_Client_Win.h"

#if (defined _MSC_VER && defined _WINDLL)
#ifdef FDFSLIENT_EXPORTS
#define FDFSCLIENT_API __declspec(dllexport)
#else
#define FDFSCLIENT_API __declspec(dllimport)
#endif
#else
#define FDFSCLIENT_API extern
#endif


#ifdef __cplusplus
extern "C"{
#endif

	FDFSCLIENT_API int fdfs_client_init(const char *conf_filename);
	FDFSCLIENT_API int fdfs_client_init_from_buffer(const char *buffer);
	FDFSCLIENT_API void fdfs_client_destroy();

	FDFSCLIENT_API ConnectionInfo *tracker_get_connection();
	FDFSCLIENT_API int tracker_query_storage_store(ConnectionInfo *pTrackerServer,
		ConnectionInfo *pStorageServer, char *group_name, 
		int *store_path_index);

	FDFSCLIENT_API int storage_upload_by_filename1(ConnectionInfo *pTrackerServer,
		ConnectionInfo *pStorageServer, const int store_path_index, 
		const char *local_filename, 
		const char *file_ext_name, const FDFSMetaData *meta_list, 
		const int meta_count, const char *group_name, char *file_id);

	FDFSCLIENT_API int storage_upload_by_filebuff1(ConnectionInfo *pTrackerServer,
		ConnectionInfo *pStorageServer, const int store_path_index, 
		const char *file_buff, const int64_t file_size, 
		const char *file_ext_name, const FDFSMetaData *meta_list, 
		const int meta_count, const char *group_name, char *file_id);

#define tracker_disconnect_server(pTrackerServer) \
	tracker_disconnect_server_ex(pTrackerServer, false)

	FDFSCLIENT_API void tracker_disconnect_server_ex(ConnectionInfo *pTrackerServer,
		const bool bForceClose);

#ifdef __cplusplus
}
#endif

#endif