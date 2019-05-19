/*
 * Created 190514 lynnl
 *
 * Hello FUSE filesystem implementation using low-level FUSE API
 *
 * see:
 *  osxfuse/filesystems/filesystems-c/hello/hello_ll.c
 *  libfuse/example/hello_ll.c
 */

#define FUSE_USE_VERSION    26
#define _FILE_OFFSET_BITS   64

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <fuse_lowlevel.h>

#include <syslog.h>

/**
 * Should only used for `char[]'  NOT `char *'
 * Assume ends with null byte('\0')
 */
#define STRLEN(s)           (sizeof(s) - 1)

#define FSNAME              "hello_fs"

#ifdef USE_TTY_LOGGING
#define LOG(fmt, ...)       (void) printf("[" FSNAME "]: " fmt "\n", ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) (void) fprintf(stderr, "[" FSNAME "]: (ERR) " fmt "\n", ##__VA_ARGS__)
#define LOG_DBG(fmt, ...)   LOG("(DBG) " fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)  LOG("(WARN) " fmt, ##__VA_ARGS__)
#else
/**
 * Do NOT use LOG_EMERG level  it'll broadcast to all users
 * macOS 10.13+ LOG_INFO, LOG_DEBUG levels rejected(log nothing)
 */
#define LOG(fmt, ...)       syslog(LOG_NOTICE, fmt "\n", ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) syslog(LOG_ERR, "[ERR] " fmt "\n", ##__VA_ARGS__)
#define LOG_DBG(fmt, ...)   syslog(LOG_NOTICE, "[DBG] " fmt "\n", ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)  syslog(LOG_WARNING, "[WARN] " fmt "\n", ##__VA_ARGS__)
#endif

#define UNUSED(arg, ...)    (void) ((void) (arg), ##__VA_ARGS__)

#define assert_nonnull(p)   assert((p) != NULL)

static const char *file_path = "/hello.txt";
static const char file_content[] = "Hello world!\n";
static const size_t file_size = STRLEN(file_content);

int main(int argc, char *argv[])
{
    struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
    struct fuse_session *se;
    struct fuse_chan *ch;
    char *mountpoint = NULL;
    int e;

    /* Setup syslog(3) */
    (void) setlogmask(LOG_UPTO(LOG_NOTICE));

    return 0;
}

