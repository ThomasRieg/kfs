#include "drivers/ide.h"

#define EXT2_ROOT_INODE 2

#define MODE_FIFO 0x1
#define MODE_CHAR 0x2
#define MODE_DIRECTORY 0x4
#define MODE_BLOCKDEV 0x6
#define MODE_REGULAR 0x8
#define MODE_SYMLINK 0xA
#define MODE_SOCKET 0xC

void ext2_mount(struct ide_partition *partition);

/* For integration in VFS later */
int read_inode(unsigned int inode_nr, unsigned int offset, char *buf, unsigned int buf_size);
int write_inode(unsigned int inode_nr, unsigned int offset, const char *buf, unsigned int buf_size);
struct VecU8 read_full_file(const char *path);
unsigned int path_to_inode(const char *path, unsigned int relative_dir_inode_nr);
bool stat_inode(unsigned int inode_nr, struct stat *out);
int getdirname(unsigned int inode_nr, char *buf, unsigned int size);
int getdents(unsigned int inode_nr, struct linux_dirent64 *ent, unsigned int count, unsigned int offset);
int unlink(const char *path, unsigned int relative_dir_inode_nr);
