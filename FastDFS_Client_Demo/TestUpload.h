#ifndef _TEST_UPLOAD_H_
#define _TEST_UPLOAD_H_

#include "test_types.h"

typedef struct {
	int bytes;  //file size
	int count;   //total file count
	int upload_count;
	int success_count;  //success upload count
	HANDLE fd;   //file description
	UINT64 time_used;  //unit: ms
	char *file_buff; //file content
} TestUploadFileInfo;

class TestUpload
{
public:
	TestUpload();
	TestUpload(int index, int file_num, int file_size);
	~TestUpload();

	int Run(UINT32 (__stdcall * func_UploadFileByID)(const BYTE *pbyFileBuff, UINT32 nFileSize, const BYTE *pbyFileExtName, BYTE *pbyFileID),
		UINT32 (__stdcall * func_UploadSlaveFileByID)(const BYTE *pbyFileBuff, UINT32 nFileSize,
						const BYTE *pbyMasterGroupName, const BYTE *pbyMasterFileName,
						const BYTE *pbyPrefixName, const BYTE *pbyFileExtName,
						BYTE *pbyFileID));
	int proccess_index;
	int load_file_contents();
	int test_init();
	int save_stats_by_overall();
	int save_stats_by_file_type();
	
private:
	time_t start_time;
	int total_count;
	int success_count;
	FILE *fpSuccess;
	FILE *fpFail;
	TestUploadFileInfo testFile;
};

#endif