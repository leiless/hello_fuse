/*
 * Created 190514 lynnl
 */

#include <stdio.h>
#include <fuse.h>

/* All FUSE functionalities NYI */
static struct fuse_operations hello_fs_op;

int main(int argc, char *argv[])
{
    return fuse_main(argc, argv, &hello_fs_op, NULL);
}

