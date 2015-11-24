#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <io.h>
#include <direct.h>
#include "test_types.h"
#include "TestDownload.h"

#define TOTAL_SECONDS 300

extern CRITICAL_SECTION m_dirOperation;

TestDownload::TestDownload()
{
	storage_count = 0;
	total_count = 0;
	success_count = 0;
	fpFail = NULL;

	proccess_index = 0;
	file_count = 0;
	file_entries = NULL;

	files[0].bytes = 1 * 1024 * 1024;
	files[0].filename = "c:\\1M";
	files[0].count = 2;
	files[0].download_count = 0;
	files[0].success_count = 0;
	files[0].time_used = 0;
}

TestDownload::TestDownload(int index, int file_num)
{
	storage_count = 0;
	total_count = 0;
	success_count = 0;
	fpFail = NULL;

	proccess_index = index;
	file_count = 0;
	file_entries = NULL;
	
	files[0].bytes = 1 * 1024 * 1024;
	files[0].filename = "c:\\1M";
	files[0].count = file_num;
	files[0].download_count = 0;
	files[0].success_count = 0;
	files[0].time_used = 0;
}

TestDownload::~TestDownload()
{
}

BYTE buffer[5*1024*1024];
int TestDownload::Run(UINT32 (__stdcall * func_DownloadFileByID)(const BYTE *pbyFileID, BYTE *pbyFileBuff, UINT32 *nFileSize))
{
	int result;
	int i;
	int file_type;
	SYSTEMTIME tStart;
	SYSTEMTIME tEnd;
	int time_used;
	char storage_ip[IP_ADDRESS_SIZE];
	memset(storage_ip, 0, sizeof(storage_ip));

	if ((result = load_file_ids()) != 0)
	{
		return result;
	}

	if ((result=test_init()) != 0)
	{
		return result;
	}

	start_time = time(NULL);
	result = 0;
	total_count = 0;
	success_count = 0;
	UINT32 file_size;
	for (i=0; i<file_count; i++)
	{
		file_type = file_entries[i].file_type;
		files[file_type].download_count++;
		total_count++;

		GetLocalTime(&tStart);
		result = func_DownloadFileByID((const BYTE*)file_entries[i].file_id, buffer, (UINT32*)&file_size);
		GetLocalTime(&tEnd);
		time_used = TIME_SUB_MS(tEnd, tStart);
		files[file_type].time_used += time_used;

		if (result == 0) //success
		{
			success_count++;
			files[file_type].success_count++;
		}
		else //fail
		{
			fprintf(fpFail, "%d %d %s %s %d %d\n", (int)tEnd.wSecond, 
				files[file_type].bytes, file_entries[i].file_id, 
				storage_ip, result, time_used);
			fflush(fpFail);
		}

		if (total_count % 10000 == 0)
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

	fclose(fpFail);

	for(int i = 0; i < file_count; i++)
	{
		free(file_entries[i].file_id);
	}
	free(file_entries);
	file_entries = NULL;

	char debugBuf[256];
	sprintf(debugBuf, "proccess %d, time used: %ds\n", proccess_index, (int)(time(NULL) - start_time));
	OutputDebugString(debugBuf);
	return result;
}

int TestDownload::save_stats_by_file_type()
{
	int k;
	char filename[64];
	FILE *fp;

	sprintf(filename, "c:\\download\\%s.%d", STAT_FILENAME_BY_FILE_TYPE, proccess_index);
	if ((fp=fopen(filename, "wb")) == NULL)
	{
		char debugBuf[256];
		sprintf(debugBuf, "open file %s fail, errno: %d, error info: %d\n", 
			filename, errno, errno);
		OutputDebugString(debugBuf);
		return errno != 0 ? errno : EPERM;
	}

	fprintf(fp, "#file_type total_count success_count time_used(ms)\n");
	for (k=0; k<FILE_TYPE_COUNT; k++)
	{
		fprintf(fp, "%s %d %d %I64d\n", \
			files[k].filename, files[k].download_count, \
			files[k].success_count, files[k].time_used);
	}

	fclose(fp);
	return 0;
}

int TestDownload::save_stats_by_overall()
{
	char filename[64];
	FILE *fp;

	sprintf(filename, "c:\\download\\%s.%d", STAT_FILENAME_BY_OVERALL, proccess_index);
	if ((fp=fopen(filename, "wb")) == NULL)
	{
		char debugBuf[256];
		sprintf(debugBuf, "open file %s fail, errno: %d, error info: %s\n", 
			filename, errno, errno);
		OutputDebugString(debugBuf);
		return errno != 0 ? errno : EPERM;
	}

	fprintf(fp, "#total_count success_count  time_used(s)\n");
	fprintf(fp, "%d %d %d\n", total_count, success_count, (int)(time(NULL) - start_time));

	fclose(fp);
	return 0;
}

int TestDownload::get_file_type_index(const int file_bytes)
{
	TestDownloadFileInfo *pFile;
	TestDownloadFileInfo *pEnd;

	pEnd = files + FILE_TYPE_COUNT;
	for (pFile=files; pFile<pEnd; pFile++)
	{
		if (file_bytes == pFile->bytes)
		{
			return pFile - files;
		}
	}

	return -1;
}

int TestDownload::load_file_ids()
{
	int i;
	int result;
	UINT64 file_size;
	int bytes;
	char filename[64];
	char *file_buff;
	char *p;
	int nLineCount;
	char *pStart;
	char *pEnd;
	char *pFind;

	sprintf(filename, "c:\\upload\\%s.%d", FILENAME_FILE_ID, proccess_index);
	if ((result=getFileContent(filename, &file_buff, &file_size)) != 0)
	{
		printf("file: "__FILE__", line: %d, " 
			"getFileContent %s fail, errno: %d, error info: %d\n", __LINE__, 
			filename, errno, errno);

		return result;
	}

	nLineCount = 0;
	p = file_buff;
	while (*p != '\0')
	{
		if (*p == '\n')
		{
			nLineCount++;
		}

		p++;
	}

	file_count = nLineCount;
	if (file_count == 0)
	{
		char debugBuf[256];
		sprintf(debugBuf, "file: "__FILE__", line: %d, " 
			"file count == 0 in file %s\n", __LINE__, filename);
		OutputDebugString(debugBuf);
		free(file_buff);
		return EINVAL;
	}

	file_entries = (FileEntry *)malloc(sizeof(FileEntry) * file_count);
	if (file_entries == NULL)
	{
		char debugBuf[256];
		sprintf(debugBuf, "file: "__FILE__", line: %d, " 
			"malloc %d bytes fail\n", __LINE__, \
			(int)sizeof(FileEntry) * file_count);
		OutputDebugString(debugBuf);
		free(file_buff);
		return ENOMEM;
	}
	memset(file_entries, 0, sizeof(FileEntry) * file_count);

	i = 0;
	p = file_buff;
	pStart = file_buff;
	while (i < file_count)
	{
		if (*p == '\n')
		{
			*p = '\0';
			pFind = strchr(pStart, ' ');
			if (pFind == NULL)
			{
				char debugBuf[256];
				sprintf(debugBuf, "file: "__FILE__", line: %d, " 
					"can't find ' ' in file %s\n", __LINE__, filename);
				OutputDebugString(debugBuf);
				result = EINVAL;
				break;
			}

			pFind++;
			pEnd = strchr(pFind, ' ');
			if (pEnd == NULL)
			{
				char debugBuf[256];
				sprintf(debugBuf, "file: "__FILE__", line: %d, " 
					"can't find ' ' in file %s\n", __LINE__, filename);
				OutputDebugString(debugBuf);
				result = EINVAL;
				break;
			}
			*pEnd = '\0';
			bytes = atoi(pFind);

			pFind = pEnd + 1;  //skip space
			pEnd = strchr(pFind, ' ');
			if (pEnd == NULL)
			{
				char debugBuf[256];
				sprintf(debugBuf, "file: "__FILE__", line: %d, " 
					"can't find ' ' in file %s\n", __LINE__, filename);
				OutputDebugString(debugBuf);
				result = EINVAL;
				break;
			}
			*pEnd = '\0';

			file_entries[i].file_type = get_file_type_index(bytes);
			if (file_entries[i].file_type < 0)
			{
				char debugBuf[256];
				sprintf(debugBuf, "file: "__FILE__", line: %d, " 
					"invalid file bytes: %d in file %s\n", __LINE__, bytes, filename);
				OutputDebugString(debugBuf);
				result = EINVAL;
				break;
			}

			file_entries[i].file_id = strdup(pFind);
			if (file_entries[i].file_id == NULL)
			{
				char debugBuf[256];
				sprintf(debugBuf, "file: "__FILE__", line: %d, " 
					"malloc %d bytes fail\n", __LINE__, \
					(int)strlen(pFind) + 1);
				OutputDebugString(debugBuf);
				result = ENOMEM;
				break;
			}

			i++;
			pStart = ++p;
		}
		else
		{
			p++;
		}
	}

	free(file_buff);

	return result;
}

int TestDownload::test_init()
{
	char filename[64];

	EnterCriticalSection(&m_dirOperation);
	if (_access("c:\\download", 0) != 0 && _mkdir("c:\\download") != 0)
	{
		LeaveCriticalSection(&m_dirOperation);
		return -1;
	}
	LeaveCriticalSection(&m_dirOperation);

	sprintf(filename, "c:\\download\\%s.%d", FILENAME_FAIL, proccess_index);
	if ((fpFail=fopen(filename, "wb")) == NULL)
	{
		char debugBuf[256];
		sprintf(debugBuf, "open file %s fail, errno: %d, error info: %s\n", 
			filename, errno, errno);
		OutputDebugString(debugBuf);
		return errno != 0 ? errno : EPERM;
	}
	return 0;
}

int TestDownload::getFileContent(const char *filename, char **buff, UINT64 *pfile_size)
{
	HANDLE hFile = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		int nRet = GetLastError();
		return -1;
	}

	int file_size = GetFileSize(hFile, NULL);
	if(file_size <= 0)
	{
		int nRet = GetLastError();
		return -1;
	}

	*buff = (char*)malloc(file_size + 1);
	if(*buff == NULL)
	{
		return -1;
	}
	DWORD dwRead = 0;
	ReadFile(hFile, *buff, file_size, &dwRead, NULL);
	(*buff)[file_size] = '\0';
	*pfile_size = file_size;
	CloseHandle(hFile);

	return 0;
}