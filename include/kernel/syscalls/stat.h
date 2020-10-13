#pragma once
#include <kernel/time/timeval.h>
#include <stdint.h>

/* File mode */
/* Read, write, execute/search by owner */
#define S_IRWXU 0000700 /* [XSI] RWX mask for owner */
#define S_IRUSR 0000400 /* [XSI] R for owner */
#define S_IWUSR 0000200 /* [XSI] W for owner */
#define S_IXUSR 0000100 /* [XSI] X for owner */
/* Read, write, execute/search by group */
#define S_IRWXG 0000070 /* [XSI] RWX mask for group */
#define S_IRGRP 0000040 /* [XSI] R for group */
#define S_IWGRP 0000020 /* [XSI] W for group */
#define S_IXGRP 0000010 /* [XSI] X for group */
/* Read, write, execute/search by others */
#define S_IRWXO 0000007 /* [XSI] RWX mask for other */
#define S_IROTH 0000004 /* [XSI] R for other */
#define S_IWOTH 0000002 /* [XSI] W for other */
#define S_IXOTH 0000001 /* [XSI] X for other */

#define S_ISUID 0004000 /* [XSI] set user id on execution */
#define S_ISGID 0002000 /* [XSI] set group id on execution */
#define S_ISVTX 0001000 /* [XSI] directory restrcted delete */

#define S_IFMT 0170000   /* type of file */
#define S_IFDIR 0040000  /* directory */
#define S_IFCHR 0020000  /* character special */
#define S_IFBLK 0060000  /* block special */
#define S_IFREG 0100000  /* regular */
#define S_IFLNK 0120000  /* symbolic link */
#define S_IFSOCK 0140000 /* socket */
#define S_IFIFO 0010000  /* fifo */

namespace influx {
namespace syscalls {
struct stat {
    uint32_t device_id;
    uint64_t inode;
    uint32_t mode;
    uint64_t hard_links_count;
    uint64_t owner_user_id;
    uint64_t owner_group_id;
    uint32_t rdevice_id;
    uint64_t size;
    time::timeval last_access;
    time::timeval last_modify;
    time::timeval creation_time;
    uint64_t fs_block_size;
    uint64_t blocks;
};
};  // namespace syscalls
};  // namespace influx