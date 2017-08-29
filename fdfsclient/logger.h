/**
* Copyright (C) 2008 Happy Fish / YuQing
*
* FastDFS may be copied only under the terms of the GNU General
* Public License V3, which may be found in the FastDFS source kit.
* Please visit the FastDFS Home Page http://www.csource.org/ for more detail.
**/

//logger.h
#ifndef LOGGER_H
#define LOGGER_H

/*
 * priorities/facilities are encoded into a single 32-bit quantity, where the
 * bottom 3 bits are the priority (0-7) and the top 28 bits are the facility
 * (0-big number).  Both the priorities and the facilities map roughly
 * one-to-one to strings in the syslogd(8) source code.  This mapping is
 * included in this file.
 * priorities (these are ordered)
 */
#define LOG_EMERG 0 /* system is unusable */
#define LOG_ALERT 1 /* action must be taken immediately */
#define LOG_CRIT 2 /* critical conditions */
#define LOG_ERR  3 /* error conditions */
#define LOG_WARNING 4 /* warning conditions */
#define LOG_NOTICE 5 /* normal but significant condition */
#define LOG_INFO 6 /* informational */
#define LOG_DEBUG 7 /* debug-level messages */

#define STDIN_FILENO  0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

#define MAX_PATH_SIZE				256
#define pthread_mutex_t int

#if defined(_MSC_VER) 
#ifdef FDFSLIENT_EXPORTS
#define FDFSLOGGER_API __declspec(dllexport)
#else
#define FDFSLOGGER_API __declspec(dllimport)
#endif
#else
#define FDFSLOGGER_API extern
#endif


#ifdef __cplusplus
extern "C" {
#endif

//log time precision
#define LOG_TIME_PRECISION_SECOND	's'  //second
#define LOG_TIME_PRECISION_MSECOND	'm'  //millisecond
#define LOG_TIME_PRECISION_USECOND	'u'  //microsecond
#define LOG_TIME_PRECISION_NONE 	'0'  //do NOT output timestamp

//log compress flags
#define LOG_COMPRESS_FLAGS_NONE       0
#define LOG_COMPRESS_FLAGS_ENABLED    1
#define LOG_COMPRESS_FLAGS_NEW_THREAD 2

struct log_context;

//log header line callback
typedef void (*LogHeaderCallback)(struct log_context *pContext);

typedef struct log_context
{
	/* log level value please see: sys/syslog.h
  	   default value is LOG_INFO */
	int log_level;

	/* default value is STDERR_FILENO */
	int log_fd;

	/* cache buffer */
	char *log_buff;

	/* string end in the cache buffer for next sprintf */
	char *pcurrent_buff;

	/* mutext lock */
	pthread_mutex_t log_thread_lock;

	/*
	rotate the log when the log file exceeds this parameter
	rotate_size > 0 means need rotate log by log file size
	*/
	__int64 rotate_size;

	/* log file current size */
	__int64 current_size;

	/* if write to buffer firstly, then sync to disk.
	   default value is false (no cache) */
	int log_to_cache;

	/* if rotate the access log */
	int rotate_immediately;

	/* if stderr to the log file */
    int take_over_stderr;

	/* if stdout to the log file */
    int take_over_stdout;

	/* time precision */
	char time_precision;

    /* if use file write lock */
    int use_file_write_lock;

    /* compress the log file use gzip command */
    short compress_log_flags;

	/* save the log filename */
	char log_filename[MAX_PATH_SIZE];

	/* the time format for rotated filename,
     * default: %Y%m%d_%H%M%S
     * */
    char rotate_time_format[32];

    /* keep days for rotated log files */
    int keep_days;

    /* log fd flags */
    int fd_flags;

    /*
     * log the header (title line) callback
     * */
    LogHeaderCallback print_header_callback;

    /*
     * compress the log files before N days
     * */
    int compress_log_days_before;
} LogContext;

FDFSLOGGER_API extern LogContext g_log_context;

/** init function using global log context
 *  return: 0 for success, != 0 fail
*/
FDFSLOGGER_API int log_init();

/** init function using global log context, take over stderr and stdout
 *  return: 0 for success, != 0 fail
*/
FDFSLOGGER_API int log_init2();

#define logEmerg   printf
#define logCrit    printf
#define logAlert   printf
#define logError   printf
#define logWarning printf
#define logNotice  printf
#define logInfo    printf
#define logDebug   printf

#ifdef __cplusplus
}
#endif

#endif

