#include "stdafx.h"
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <direct.h>
#include <time.h>
#include "TestUpload.h"
#include "test_types.h"
#include "FastDFS_Client_Win.h"

extern CRITICAL_SECTION m_dirOperation;

TestUpload::TestUpload()
{
	proccess_index = 0;
	total_count = 0;
	success_count = 0;
	fpSuccess = NULL;
	fpFail = NULL;
	
	testFile.bytes = 1 * 1024 * 1024;
	testFile.count = 1;
	testFile.upload_count = 0;
	testFile.success_count = 0;
	testFile.fd = NULL;
	testFile.time_used = 0;
	testFile.file_buff = NULL;
}

TestUpload::TestUpload(int index, int file_num, int file_size)
{
	proccess_index = index;
	total_count = 0;
	success_count = 0;
	fpSuccess = NULL;
	fpFail = NULL;
	
	testFile.bytes = file_size;
	testFile.count = file_num;
	testFile.upload_count = 0;
	testFile.success_count = 0;
	testFile.fd = NULL;
	testFile.time_used = 0;
	testFile.file_buff = NULL;
}

TestUpload::~TestUpload()
{
	delete testFile.file_buff;
}

UINT32 GetGroupName(const BYTE *pbyData, BYTE *pbyRet)
{
	const BYTE *p = pbyData;
	UINT32 nLen = strlen((const char*)pbyData) + 1;
	for(int i = 0; i < nLen; i++)
	{
		if(*p == '/' || *p == '\0')
		{
			pbyRet[i] = '\0';
			break;
		}
		pbyRet[i] = *p;
		p++;
	}
	return 0;
}

UINT32 GetMasterFileName(const BYTE *pbyData, BYTE *pbyRet)
{
	const BYTE *p = pbyData;
	UINT32 nLen = strlen((const char*)pbyData) + 1;
	while(1)
	{
		if(*p == '/')
			break;
		p++;
	}
	memcpy(pbyRet, p + 1, nLen - (p - pbyData));
	return 0;
}

int TestUpload::Run(UINT32 (__stdcall * func_UploadFileByID)(const BYTE *pbyFileBuff, UINT32 nFileSize, const BYTE *pbyFileExtName, BYTE *pbyFileID),
					UINT32 (__stdcall * func_UploadSlaveFileByID)(const BYTE *pbyFileBuff, UINT32 nFileSize,
						const BYTE *pbyMasterGroupName, const BYTE *pbyMasterFileName,
						const BYTE *pbyPrefixName, const BYTE *pbyFileExtName,
						BYTE *pbyFileID))
{
	int result;
	int upload_count;
	int rand_num;
	int file_index;
	char file_id[256];
	int i;
	int time_used;
	SYSTEMTIME tStart;
	SYSTEMTIME tEnd;


	if ((result = load_file_contents()) != 0)
	{
		return result;
	}

	if ((result=test_init()) != 0)
	{
		return result;
	}

	upload_count = testFile.count;

	if (upload_count == 0)
	{
		return EINVAL;
	}

	memset(file_id, 0, sizeof(file_id));

	start_time = time(NULL);
	srand(0);
	result = 0;
	total_count = 0;
	success_count = 0;
	while (total_count < upload_count)
	{
		total_count++;
		GetLocalTime(&tStart);

		result = func_UploadFileByID((const BYTE*)testFile.file_buff, testFile.bytes, (const BYTE*)"jpg", (BYTE*)file_id);
	
		GetLocalTime(&tEnd);
		time_used = TIME_SUB_MS(tEnd, tStart);
		testFile.time_used += time_used;
		testFile.upload_count++;
		if (result == 0) //success
		{
			success_count++;
			testFile.success_count++;

			fprintf(fpSuccess, "%d %d %s %d\n", 
				(int)tEnd.wSecond, testFile.bytes, 
				file_id, time_used);
		}
		else //fail
		{
			fprintf(fpFail, "%d %d %d %d\n", (int)tEnd.wSecond, 
				testFile.bytes, result, time_used);
			fflush(fpFail);
		}


		if (total_count % 100 == 0)
		{
			if ((result=save_stats_by_overall()) != 0)
			{
				break;
			}
			if ((result=save_stats_by_file_type()) != 0)
			{
				break;
			}
		}

	}

	save_stats_by_overall();
	save_stats_by_file_type();

	fclose(fpSuccess);
	fclose(fpFail);

	return result;
}

int TestUpload::save_stats_by_file_type()
{
	int k;
	char filename[64];
	FILE *fp;

	sprintf(filename, "c:\\upload\\%s.%d", STAT_FILENAME_BY_FILE_TYPE, proccess_index);
	if ((fp=fopen(filename, "wb")) == NULL)
	{
		return errno != 0 ? errno : EPERM;
	}

	fprintf(fp, "#file_size total_count success_count time_used(ms)\n");
	fprintf(fp, "%d %d %d %I64d\n", \
		testFile.bytes, testFile.upload_count, \
		testFile.success_count, testFile.time_used);

	fclose(fp);
	return 0;
}

int TestUpload::save_stats_by_overall()
{
	char filename[64];
	FILE *fp;

	sprintf(filename, "c:\\upload\\%s.%d", STAT_FILENAME_BY_OVERALL, proccess_index);
	if ((fp=fopen(filename, "wb")) == NULL)
	{
		return errno != 0 ? errno : EPERM;
	}

	fprintf(fp, "#total_count success_count  time_used(s)\n");
	fprintf(fp, "%d %d %d\n", total_count, success_count, (int)(time(NULL) - start_time));

	fclose(fp);
	return 0;
}

int TestUpload::load_file_contents()
{
	int i;
	//int result;

	testFile.file_buff = new char[testFile.bytes];
	if(testFile.file_buff == NULL)
	{
		return -1;
	}
	memset(testFile.file_buff, 0x1, testFile.bytes);

	return 0;
}

int TestUpload::test_init()
{
	char filename[64];

	EnterCriticalSection(&m_dirOperation);
	if (_access("c:\\upload", 0) != 0 && _mkdir("c:\\upload") != 0)
	{
		LeaveCriticalSection(&m_dirOperation);
		return -1;
	}
	LeaveCriticalSection(&m_dirOperation);

	sprintf(filename, "c:\\upload\\%s.%d", FILENAME_FILE_ID, proccess_index);
	if ((fpSuccess=fopen(filename, "wb")) == NULL)
	{
		LeaveCriticalSection(&m_dirOperation);
		return errno != 0 ? errno : EPERM;
	}

	sprintf(filename, "c:\\upload\\%s.%d", FILENAME_FAIL, proccess_index);
	if ((fpFail=fopen(filename, "wb")) == NULL)
	{
		LeaveCriticalSection(&m_dirOperation);
		return errno != 0 ? errno : EPERM;
	}

	return 0;
}
