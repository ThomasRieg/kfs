#include "drivers/ide.h"

#define EXT2_ROOT_INODE 2

void ext2_test(struct ide_partition *partition);
int read_inode(unsigned int inode_nr, unsigned int offset, char *buf, unsigned int buf_size);
struct VecU8 read_full_file(const char *path);
unsigned int path_to_inode(const char *path, unsigned int relative_dir_inode_nr);
