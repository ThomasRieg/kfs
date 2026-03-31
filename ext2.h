#include "drivers/ide.h"
#include "fd/fd.h"

#define EXT2_ROOT_INODE 2

#define MODE_FIFO 0x1
#define MODE_CHAR 0x2
#define MODE_DIRECTORY 0x4
#define MODE_BLOCKDEV 0x6
#define MODE_REGULAR 0x8
#define MODE_SYMLINK 0xA
#define MODE_SOCKET 0xC

void ext2_mount(struct ide_partition *partition);

int ext2_open(const char *path, unsigned int dir_inode, unsigned int flags, unsigned int mode, t_file **fp);

/* For integration in VFS later */
int read_inode(unsigned int inode_nr, unsigned int offset, char *buf, unsigned int buf_size);
int ext2_write_inode_contents(unsigned int inode_nr, unsigned int offset, const unsigned char *buf, unsigned int buf_size);
struct VecU8 read_full_file(const char *path);
unsigned int path_to_inode(const char *path, unsigned int relative_dir_inode_nr);
bool stat_inode(unsigned int inode_nr, struct stat *out);
int ext2_getdirname(unsigned int inode_nr, char *buf, unsigned int size);
int ext2_getdents(unsigned int inode_nr, struct linux_dirent64 *ent, unsigned int count, unsigned int offset);
int ext2_unlink(const char *path, unsigned int relative_dir_inode_nr);
int ext2_mkdir(const char *path, unsigned int relative_dir_inode_nr);
