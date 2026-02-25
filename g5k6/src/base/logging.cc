#include "base/logging.h"
#include "base/app_common_defs.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <stdarg.h>

FILE* g_log_file = NULL;
static bool g_log_file_enabled = false;
static char g_log_file_path[256] = {0};
static const size_t kMaxLogFileSize = 512 * 1024;  // 512KB

static long get_file_size(const char* filepath) {
    struct stat st;
    if (stat(filepath, &st) == 0) {
        return st.st_size;
    }
    return -1;
}

static void rotate_log_file(const char* log_file_path) {
    remove(log_file_path);
}

static void check_and_rotate_log_file(void) {
    if (g_log_file_path[0] == '\0') {
        return;
    }

    long file_size = get_file_size(g_log_file_path);
    if (file_size >= (long)kMaxLogFileSize) {
        if (g_log_file != NULL) {
            fclose(g_log_file);
            g_log_file = NULL;
        }

        rotate_log_file(g_log_file_path);

        g_log_file = fopen(g_log_file_path, "w");
        if (g_log_file != NULL) {
            setvbuf(g_log_file, NULL, _IOLBF, 0);
        }
    }
}

int LogInit(const char* log_file_path) {
    if (log_file_path == NULL || log_file_path[0] == '\0') {
        g_log_file_enabled = false;
        return -1;
    }

    if (g_log_file != NULL) {
        fclose(g_log_file);
        g_log_file = NULL;
    }

    char final_path[256] = {0};
    strncpy(final_path, log_file_path, sizeof(final_path) - 1);
    final_path[sizeof(final_path) - 1] = '\0';

    struct stat st;
    size_t len = strlen(final_path);

    if (stat(final_path, &st) != 0 || !S_ISDIR(st.st_mode)) {
        g_log_file_enabled = false;
        g_log_file_path[0] = '\0';
        return -1;
    }

    if (len > 0 && final_path[len - 1] != '/') {
        snprintf(final_path + len, sizeof(final_path) - len, "/%s", LOG_FILE_NAME);
    } else {
        snprintf(final_path + len, sizeof(final_path) - len, "%s", LOG_FILE_NAME);
    }

    strncpy(g_log_file_path, final_path, sizeof(g_log_file_path) - 1);
    g_log_file_path[sizeof(g_log_file_path) - 1] = '\0';

    long file_size = get_file_size(final_path);
    if (file_size >= (long)kMaxLogFileSize) {
        rotate_log_file(final_path);
        g_log_file = fopen(final_path, "w");
    } else {
        g_log_file = fopen(final_path, "a");
    }

    if (g_log_file == NULL) {
        g_log_file_path[0] = '\0';
        g_log_file_enabled = false;
        return -1;
    }

    setvbuf(g_log_file, NULL, _IOLBF, 0);

    g_log_file_enabled = true;
    return 0;
}

void LogClose(void) {
    if (g_log_file != NULL) {
        fflush(g_log_file);
        fclose(g_log_file);
        g_log_file = NULL;
    }
    g_log_file_path[0] = '\0';
    g_log_file_enabled = false;
}

bool IsLogFileEnabled(void) {
    return g_log_file_enabled;
}

void log_write_internal(const char* color, const char* time_str, int time_ms, const char* filename, int line, const char* fmt, ...) {
    if (!g_log_file_enabled) {
        return;
    }

    char log_buffer[1024] = {0};
    va_list args;
    va_start(args, fmt);
    int len = snprintf(log_buffer, sizeof(log_buffer), "[%s.%03d] [%s:%d] ",
                       time_str, time_ms, filename, line);
    if (len > 0 && len < (int)sizeof(log_buffer)) {
        vsnprintf(log_buffer + len, sizeof(log_buffer) - len, fmt, args);
    }
    va_end(args);

    printf("%s%s\033[0m\n", color, log_buffer);

    check_and_rotate_log_file();

    if (g_log_file != NULL) {
        fprintf(g_log_file, "%s\n", log_buffer);
        fflush(g_log_file);
    }
}