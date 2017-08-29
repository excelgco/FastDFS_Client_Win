#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <assert.h>
#include <windows.h>
#include "fdfs_client.h"

int fdfs_client_init(const char *conf_filename)
{
	int ret = 0;
	char *ini = NULL;
	while (NULL == ini)
	{
		struct _stat st;
		int result = _stat(conf_filename, &st);
		if (result != 0)
		{
			printf("error: unable to retrieve \"%s\" size.\n",
				conf_filename);
			break;
		}

		FILE *fp = fopen(conf_filename, "rb");
		if (NULL == fp)
		{
			printf("error: unable to open \"%s\" as file.\n",
				conf_filename);
			break;
		}
		ini = (char*)malloc(st.st_size + 1);
		if (NULL == ini)
		{
			printf("error: unable to alloc %I64d bytes.\n",
				st.st_size + 1);
		    fclose(fp);
			break;
		}
		size_t readed = fread(ini, 1, st.st_size, fp);
		assert(readed == st.st_size);
		if (readed != st.st_size)
		{
			printf("error: readed bytes %I64d is not same as \"%s\" size: %I64d\n",
				readed, conf_filename, st.st_size);
			free(ini);
			ini = NULL;
		}
		fclose(fp);
		ini[st.st_size] = '\0';
		break;
	}
	if (NULL == ini)
	{
		return EINTR;
	}
	ret = fdfs_client_init_from_buffer(ini);
	free(ini);
	return ret;
}

int fdfs_client_init_from_buffer(const char *buffer)
{
	int ret = 0;
	const char *line = buffer;
    const char *tracker = 0;
	ServerAddress addrs[FDFS_MAX_TRACKERS];
	int count = 0;
    while (count < FDFS_MAX_TRACKERS)
    {
        tracker = strstr(line, "tracker_server");
        if (!tracker) {
            break;
        }
        //comment out?
		bool ignore = false;
        const char *from = tracker - 1;
        for (; from >= line; --from) {
            if (*from == '#') {
				//comment
				ignore = true;
                break;
            }
			if (*from == '\n') {
				break;
			}
			if (!isspace(*from)) {
				//prefixed by something, key in ini is not "tracker_server"
				ignore = true;
				break;
			}
		}
        if (!ignore) {//not comment
            tracker += sizeof("tracker_server") - 1;
            //skip '='
            const char *from = tracker;
            for (; *from != '\0'; ++from) {
                if (*from == '=') {
                    tracker = from + 1;
                    break;
                }
                if (*from == '\n') {
					ignore = true;
                    break;
                }
				if (!isspace(*from)) {
					//suffixed by something, key in ini is not "tracker_server"
					ignore = true;
					break;
				}
			}
        }
		if (!ignore) {
			sscanf(tracker, "%[^:]:%d",
				addrs[count].szIP,
				&addrs[count].nPort);
			++count;
		}
		const char *next = strchr(tracker, '\n');
        if (!next) {
            break;
        }
        line = next + 1;
    }
	if (count == 0) {
		ret = EFAULT;
		printf("no tracker in client config file\n"); 
	}
	ret = FDFSC_Initialize(addrs, count, 0);
	return ret;
}

void fdfs_client_destroy()
{
	//int ret = 0;
    FDFSC_UnInitialize();
}

ConnectionInfo *tracker_get_connection()
{
	ConnectionInfo *pTracker = tracker_get_connection_win();
	return pTracker;
}

int tracker_query_storage_store(ConnectionInfo *pTrackerServer,
		ConnectionInfo *pStorageServer, char *group_name, 
		int *store_path_index)
{
	int ret = 0;
    ret = tracker_query_storage_store_win(pTrackerServer,
        pStorageServer, group_name,
        store_path_index);
	return ret;
}

#define READ_BLOCK_SIZE (1024 * 1024)

int storage_upload_by_filename1(ConnectionInfo *pTrackerServer, 
		ConnectionInfo *pStorageServer, const int store_path_index, 
		const char *local_filename, 
		const char *file_ext_name, const FDFSMetaData *meta_list, 
		const int meta_count, const char *group_name, char *file_id)
{
	int ret = 0;
    ret = storage_upload_by_filename1_win(pTrackerServer,
		pStorageServer, store_path_index,
		local_filename,
		file_ext_name, meta_list,
		meta_count, group_name, file_id);
	return ret;
}

int storage_upload_by_filebuff1(ConnectionInfo *pTrackerServer, 
	ConnectionInfo *pStorageServer, const int store_path_index, 
	const char *file_buff, const int64_t file_size, 
	const char *file_ext_name, const FDFSMetaData *meta_list, 
	const int meta_count, const char *group_name, char *file_id)
{
	int ret = 0;
	ret = storage_upload_by_filebuff1_win(pTrackerServer,
		pStorageServer, store_path_index,
		file_buff, file_size,
		file_ext_name, meta_list,
		meta_count, group_name, file_id);
	return ret;
}

void tracker_disconnect_server_ex(ConnectionInfo *pTrackerServer,
	const bool bForceClose) {
    //nothin to do
}

