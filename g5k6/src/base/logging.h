#ifndef BASE_LOGGING_H_
#define BASE_LOGGING_H_

#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// 日志文件初始化函数
int LogInit(const char* log_file_path);
void LogClose(void);
bool IsLogFileEnabled(void);

// 全局日志文件指针（在 logging.cc 中定义）
extern FILE* g_log_file;

#define MACRO_LOG_BLACK "\033[30m"
#define MACRO_LOG_RED "\033[31m"
#define MACRO_LOG_GREEN "\033[32m"
#define MACRO_LOG_YELLOW "\033[33m"
#define MACRO_LOG_BLUE "\033[34m"
#define MACRO_LOG_PURPLE "\033[35m"
#define COLOR_CYAN    "\033[36m"
#define MACRO_LOG_WHITE "\033[37m"
#define MACRO_LOG_END "\033[0m\n"

typedef enum {
    LOG_SYNC_SEND = 0,
    LOG_SYNC_RECV = 1,
    LOG_ASYN_SEND = 2,
    LOG_ASYN_RECV = 3,
    LOG_FLAG_BUTT
} LOG_FLAG_E;

typedef enum {
    LOG_TARGET_NULL = 0,
    LOG_TARGET_SYSLOG = 1,
    LOG_TARGET_APPLOG = 2,
    LOG_TARGET_STDOUT = 4,
    LOG_TARGET_BUTT
} LOG_TARGET_E;


typedef enum {
    LOG_SUCCESS = 0,
    LOG_CRITICAL = 1,
    LOG_ERROR = 2,
    LOG_WARN = 3,
    LOG_NOTICE = 4,
    LOG_INFO = 5,
    LOG_DEBUG = 6,
    LOG_TRACE = 7,
    LOG_BUTT
} LOG_LEVEL_E;

#define LOG_LEVEL_USER (LOG_DEBUG)


#define FILENAME(x) (strrchr(x, '/') ? strrchr(x, '/')+1 : x)


void log_write_internal(const char* color, const char* time_str, int time_ms, const char* filename, int line, const char* fmt, ...);


#define LOGSTR(level, clor, fmt,...) \
  if (level <= LOG_LEVEL_USER) { \
          char __time_str[20] = { 0 };                                               \
    struct timespec __tsp = { 0 };                                             \
    time_t __now_sec;                                                          \
    time_t __now_ms;                                                           \
    clock_gettime(CLOCK_REALTIME, &__tsp);                                     \
    __now_ms = (time_t)(__tsp.tv_nsec / 1000000);                              \
    __now_sec = __tsp.tv_sec;                                                  \
    strftime(__time_str, sizeof(__time_str), "%T", localtime(&__now_sec));  \
    log_write_internal(clor, __time_str, (int)__now_ms, FILENAME(__FILE__), __LINE__, fmt, ##__VA_ARGS__); \
    }


#define LOG_S(fmt, ...) LOGSTR(LOG_SUCCESS, MACRO_LOG_GREEN, "S: " fmt, ##__VA_ARGS__)
#define LOG_C(fmt, ...) LOGSTR(LOG_CRITICAL, MACRO_LOG_WHITE, "C: " fmt, ##__VA_ARGS__)
#define LOG_W(fmt, ...) LOGSTR(LOG_WARN, MACRO_LOG_YELLOW, "W: " fmt, ##__VA_ARGS__)
#define LOG_E(fmt, ...) LOGSTR(LOG_ERROR, MACRO_LOG_RED, "E: " fmt, ##__VA_ARGS__)
#define LOG_N(fmt, ...) LOGSTR(LOG_NOTICE, COLOR_CYAN, "N: " fmt, ##__VA_ARGS__)


#if LOG_LEVEL_USER < LOG_INFO
#define LOG_I(fmt, ...)
#define LOG_D(fmt, ...)
#define LOG_T(fmt, ...)
#else
#define LOG_I(fmt, ...) LOGSTR(LOG_INFO, MACRO_LOG_WHITE, "I: " fmt, ##__VA_ARGS__)
#define LOG_D(fmt, ...) LOGSTR(LOG_DEBUG, MACRO_LOG_BLUE, "D: " fmt, ##__VA_ARGS__)
#define LOG_T(fmt, ...) LOGSTR(LOG_TRACE, MACRO_LOG_BLUE, "T: " fmt, ##__VA_ARGS__)
#endif

#define LOGS LOG_S
#define LOGC LOG_C
#define LOGD LOG_D
#define LOGT LOG_T
#define LOGI LOG_I
#define LOGW LOG_W
#define LOGE LOG_E

#define LOG_M_S(tag, fmt, ...) LOGSTR(LOG_CRITICAL, MACRO_LOG_GREEN, "S %s: " fmt, tag, ##__VA_ARGS__)
#define LOG_M_C(tag, fmt, ...) LOGSTR(LOG_CRITICAL, MACRO_LOG_WHITE, "C %s: " fmt, tag, ##__VA_ARGS__)
#define LOG_M_E(tag, fmt, ...) LOGSTR(LOG_ERROR, MACRO_LOG_RED, "E %s: " fmt, tag, ##__VA_ARGS__)
#define LOG_M_W(tag, fmt, ...) LOGSTR(LOG_WARN, MACRO_LOG_YELLOW, "W %s: " fmt, tag, ##__VA_ARGS__)
#define LOG_M_I(tag, fmt, ...) LOGSTR(LOG_INFO, MACRO_LOG_WHITE, "I %s: " fmt, tag, ##__VA_ARGS__)
#define LOG_M_D(tag, fmt, ...) LOGSTR(LOG_DEBUG, MACRO_LOG_BLUE, "D %s: " fmt, tag, ##__VA_ARGS__)
#define LOG_M_N(tag, fmt, ...) LOGSTR(LOG_NOTICE, COLOR_CYAN, "N %s: " fmt, tag, ##__VA_ARGS__)

#if LOG_LEVEL_USER < LOG_INFO
#define HEX_PRINT(buffer, length)
#else
#define HEX_PRINT(buffer, length) \
  { \
    printf("\n"); \
    for (int index = 0; index < length; ++index) { \
      printf("%02x ", buffer[index]); \
    } \
    printf("\n"); \
  }
#endif

#ifdef __cplusplus
}
#endif

#endif //BASE_LOGGING_H_