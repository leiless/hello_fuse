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

static int hello_stat(fuse_ino_t ino, struct stat *stbuf)
{
    assert_nonnull(stbuf);

    stbuf->st_ino = ino;
    switch (ino) {
    case 1:
        stbuf->st_mode = S_IFDIR | 0755;    /* rwxr-xr-x */
        stbuf->st_nlink = 2;
        break;

    case 2:
        stbuf->st_mode = S_IFREG | 0444;    /* r--r--r-- */
        stbuf->st_nlink = 1;
        stbuf->st_size = file_size;
        break;

    default:
        return -1;
    }

    return 0;
}

static void hello_ll_lookup(
        fuse_req_t req,
        fuse_ino_t parent,
        const char *name)
{
    struct fuse_entry_param param;
    int e;

    assert_nonnull(req);
    assert_nonnull(name);

    _LOG_DBG("lookup()  parent: %#lx name: %s", parent, name);

    if (parent != 1 || strcmp(name, file_path) != 0) {
        fuse_reply_err(req, ENOENT);
        return;
    }

    (void) memset(&e, 0, sizeof(e));
    param.ino = 2;              /* see: hello_stat() */
    param.attr_timeout = 1.0;
    param.entry_timeout = 1.0;
    (void) hello_stat(param.ino, &param.attr);

    e = fuse_reply_entry(req, &param);
    if (e != 0) _LOG_ERR("fuse_reply_entry() fail  errno: %d", -e);
}

static void hello_ll_getattr(
        fuse_req_t req,
        fuse_ino_t ino,
        struct fuse_file_info *fi)
{
    struct stat stbuf;
    int e;

    _LOG_DBG("getattr()  ino: %#lx fi->flags: %#x", ino, fi->flags);

    (void) memset(&stbuf, 0, sizeof(stbuf));

    if (hello_stat(ino, &stbuf) == 0) {
        e = fuse_reply_attr(req, &stbuf, 1.0);
        if (e != 0) _LOG_ERR("fuse_reply_attr() fail  errno: %d", -e);
    } else {
        e = fuse_reply_err(req, ENOENT);
        if (e != 0) _LOG_ERR("fuse_reply_err() fail  errno: %d", -e);
    }
}

struct dirbuf {
    char *p;
    size_t size;
};

static void dirbuf_add(
        fuse_req_t req,
        struct dirbuf *b,
        const char *name,
        fuse_ino_t ino)
{
    struct stat stbuf;
    size_t oldsize;
    char *newp;

    assert_nonnull(req);
    assert_nonnull(b);
    assert_nonnull(name);

    oldsize = b->size;

    b->size += fuse_add_direntry(req, NULL, 0, name, NULL, 0);

    newp = realloc(b->p, b->size);
    assert_nonnull(newp);
    b->p = newp;

    (void) memset(&stbuf, 0, sizeof(stbuf));
    stbuf.st_ino = ino;

    (void) fuse_add_direntry(req, b->p + oldsize, b->size - oldsize,
                                name, &stbuf, b->size);
}

#ifndef MIN
#define MIN(a, b)   (((a) < (b)) ? (a) : (b))
#endif

static int reply_buf_limited(
        fuse_req_t req,
        const char *buf,
        size_t bufsize,
        off_t off,
        size_t maxsize)
{
    assert(off >= 0);

    if ((size_t) off < bufsize)
        return fuse_reply_buf(req, buf + off, MIN(bufsize - off, maxsize));

    return fuse_reply_buf(req, NULL, 0);
}

static void hello_ll_readdir(
        fuse_req_t req,
        fuse_ino_t ino,
        size_t size,
        off_t off,
        struct fuse_file_info *fi)
{
    struct dirbuf b;
    int e;

    LOG_DBG("readdir()  ino: %#lx size: %zu off: %lld fi->flags: %#x",
                        ino, size, off, fi->flags);

    if (ino != 1) {     /* If not root directory */
        fuse_reply_err(req, ENOENT);
        return;
    }

    (void) memset(&b, 0, sizeof(b));

    dirbuf_add(req, &b, ".", 1);                /* Root directory */
    dirbuf_add(req, &b, "..", 1);               /* Parent of root is itself */
    dirbuf_add(req, &b, file_path + 1, 2);      /* The only regular file */

    e = reply_buf_limited(req, b.p, b.size, off, size);
    if (e != 0) {
        _LOG_ERR("reply_buf_limited() fail  errno: %d size: %zu off: %lld max: %zu",
                    -e, b.size, off, size);
    }

    free(b.p);
}

static void hello_ll_open(
        fuse_req_t req,
        fuse_ino_t ino,
        struct fuse_file_info *fi)
{
    int e;

    _LOG_DBG("open()  ino: %#lx fi->flags: %#x", ino, fi->flags);

    if (ino != 2) {
        e = fuse_reply_err(req, EISDIR);
    } else if ((fi->flags & O_ACCMODE) != O_RDONLY) {
        e = fuse_reply_err(req, EACCES);
    } else {
        e = fuse_reply_open(req, fi);
    }

    assert(e == 0);
}

static void hello_ll_read(
        fuse_req_t req,
        fuse_ino_t ino,
        size_t size,
        off_t off,
        struct fuse_file_info *fi)
{
    int e;
    _LOG_DBG("read()  ino: %#lx size: %zu off: %lld fi->flags: %#x",
                        ino, size, off, fi->flags);

    assert(ino == 2);
    e = reply_buf_limited(req, file_content, file_size, off, size);
    if (e != 0) _LOG_ERR("reply_buf_limited() fail  errno: %d", -e);
}

static const struct fuse_lowlevel_ops hello_ll_ops = {
    .lookup = hello_ll_lookup,
    .getattr = hello_ll_getattr,
    .readdir = hello_ll_readdir,
    .open = hello_ll_open,
    .read = hello_ll_read,
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

