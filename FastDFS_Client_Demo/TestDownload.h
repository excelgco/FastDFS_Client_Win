#ifndef _TEST_DOWNLOAD_H_
#define _TEST_DOWNLOAD_H_

#include "test_types.h"

typedef struct {
	int bytes;  //file size
	char *filename;
	int count;   //total file count
	int download_count;
	int success_count;  //success upload count
	UINT64 time_used;  //unit: ms
} TestDownloadFileInfo;

class TestDownload
{
public:
	TestDownload();
	TestDownload(int index, int file_num);
	~TestDownload();

	int Run(UINT32 (__stdcall * func_DownloadFileByID)(const BYTE *pbyFileID, BYTE *pbyFileBuff, UINT32 *nFileSize));

	int load_file_ids();
	int test_init();
	int save_stats_by_overall();
	int save_stats_by_file_type();
	int get_file_type_index(const int file_bytes);
	int getFileContent(const char *filename, char **buff, UINT64 *file_size);

private:
	int storage_count;
	time_t start_time;
	int total_count;
	int success_count;
	FILE *fpFail;

	int proccess_index;
	int file_count;
	FileEntry *file_entries;
	TestDownloadFileInfo files[FILE_TYPE_COUNT];
};

#endif