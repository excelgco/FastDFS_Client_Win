/**
* Copyright (C) 2008 Happy Fish / YuQing
*
* FastDFS may be copied only under the terms of the GNU General
* Public License V3, which may be found in the FastDFS source kit.
* Please visit the FastDFS Home Page http://www.csource.org/ for more detail.
**/

#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "logger.h"

#ifndef LINE_MAX
#define LINE_MAX 2048
#endif

#define LOG_BUFF_SIZE    64 * 1024

#define NEED_COMPRESS_LOG(flags) ((flags & LOG_COMPRESS_FLAGS_ENABLED) != 0)
#define COMPRESS_IN_NEW_THREAD(flags) ((flags & LOG_COMPRESS_FLAGS_NEW_THREAD) != 0)

#define GZIP_EXT_NAME_STR  ".gz"
#define GZIP_EXT_NAME_LEN  (sizeof(GZIP_EXT_NAME_STR) - 1)

extern LogContext g_log_context = {LOG_INFO, STDERR_FILENO, NULL};

int log_init_ex(LogContext *pContext);

int log_init()
{
	if (g_log_context.log_buff != NULL)
	{
		return 0;
	}

	return log_init_ex(&g_log_context);
}

int log_init2()
{
    int result;
    if ((result=log_init()) != 0) {
        return result;
    }

    return 0;
}

int log_init_ex(LogContext *pContext)
{
	return 0;
}
