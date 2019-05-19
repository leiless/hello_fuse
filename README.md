## hello_fuse - A hello FUSE for macOS filesystem implementation

`hello_fuse` is a trivial [FUSE](https://en.wikipedia.org/wiki/Filesystem_in_Userspace) filesystem implementation use [FUSE for macOS](https://osxfuse.github.io/) C-binding SDK.

This filesystem have only one regular file in the root directory, and grant `O_RDONLY` access only.

Code mainly taken from [osxfuse/filesystems/filesystems-c/hello](https://github.com/osxfuse/filesystems/tree/master/filesystems-c/hello), used for learning purpose...

### Compile

Simple [`make(1)`](x-man-page://1/make) let you build `DEBUG` version of `hello_fuse`, to build release version, please use `release` target.

### Mount `hello_fuse`

```shell
mkdir mount_point
./hello_fs-debug mount_point
```

For a full list of mount options, please specify `--help` for `hello-debug`

### Capture logs issued by `hello_fuse`

`hello_fuse` uses [`syslog(3)`](x-man-page://3/syslog) for logging, to see its output:

```shell
syslog -F '$((Time)(JZ)) $Host <$((Level)(str))> $(Sender)[$(PID)]: $Message' -w 0 -k Sender hello_fs-debug
```

If you uses `release` version, replace `hello_fs-debug` with `hello_fs`

### Unmount `hello_fuse`

```shell
umount mount_point
```

### TODO

* Implement `hello_fuse` use [`<fuse_lowlevel.h>`](https://github.com/osxfuse/fuse/blob/master/include/fuse_lowlevel.h) APIs

* Use new [`os_log(3)`](x-man-page://3/os_log) API for macOS >= 10.12

* Add more files(of different types, contents, attributes) to explore *FUSE for macOS*

### *References*

[osxfuse/filesystems/filesystems-c/hello](https://github.com/osxfuse/filesystems/tree/master/filesystems-c/hello)
