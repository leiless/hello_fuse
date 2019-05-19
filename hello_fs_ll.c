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
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <fuse_lowlevel.h>

#include <syslog.h>

/**
 * Should only used for `char[]'  NOT `char *'
 * Assume ends with null byte('\0')
 */
#define STRLEN(s)           (sizeof(s) - 1)

#define FSNAME              "hello_fs_ll"

#define _LOG(fmt, ...)          (void) printf(FSNAME ": " fmt "\n", ##__VA_ARGS__)
#define _LOG_ERR(fmt, ...)      (void) fprintf(stderr, FSNAME ": [ERR] " fmt "\n", ##__VA_ARGS__)
#define _LOG_DBG(fmt, ...)      LOG("[DBG] " fmt, ##__VA_ARGS__)
#define _LOG_WARN(fmt, ...)     LOG("[WARN] " fmt, ##__VA_ARGS__)

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

static const struct fuse_lowlevel_ops hello_ll_ops = {

};

int main(int argc, char *argv[])
{
    struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
    struct fuse_chan *ch;
    struct fuse_session *se;
    char *mountpoint = NULL;
    int e;

    /* Setup syslog(3) */
    (void) setlogmask(LOG_UPTO(LOG_NOTICE));

    e = fuse_parse_cmdline(&args, &mountpoint, NULL, NULL);
    if (e == -1) {
        _LOG_ERR("fuse_parse_cmdline() fail");
        e = 1;
        goto out_fail;
    } else if (mountpoint == NULL) {
        if (argc == 1) _LOG_ERR("no mountpoint  -h for help");
        e = 2;
        goto out_args;
    }

    assert_nonnull(mountpoint);
    _LOG("mountpoint: %s", mountpoint);

    ch = fuse_mount(mountpoint, &args);
    if (ch == NULL) {
        _LOG_ERR("fuse_mount() fail");
        e = 3;
        goto out_chan;
    }

    se = fuse_lowlevel_new(&args, &hello_ll_ops, sizeof(hello_ll_ops), NULL /* user data */);
    if (se == NULL) {
        _LOG_ERR("fuse_lowlevel_new() fail");
        e = 4;
        goto out_se;
    }

    if (fuse_set_signal_handlers(se) != -1) {
        fuse_session_add_chan(se, ch);

        if (fuse_session_loop(se) == -1) {
            e = 5;
            _LOG_ERR("fuse_session_loop() fail");
        }

        fuse_remove_signal_handlers(se);
        fuse_session_remove_chan(ch);
    } else {
        e = 6;
        _LOG_ERR("fuse_set_signal_handlers() fail");
    }

    fuse_session_destroy(se);
out_se:
    fuse_unmount(mountpoint, ch);
out_chan:
    free(mountpoint);
out_args:
    fuse_opt_free_args(&args);
out_fail:
    return e;
}

