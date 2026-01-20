#include "drivers/ide.h"

void ext2(struct ide_drive *drive, unsigned int lba_start, unsigned int total_sectors);

union ext2_super_block {
	struct {
		unsigned int total_inodes;
		unsigned int total_blocks;
		unsigned int reserved_blocks; // for root only
		unsigned int free_blocks;
		unsigned int free_inodes;
		unsigned int starting_block;
		unsigned int block_size; // bytes = 1024 << .block_size
		unsigned int fragment_size; // bytes = 1024 << .fragment_size
		unsigned int blocks_in_group;
		unsigned int fragments_in_group;
		unsigned int inodes_in_group;
		unsigned int last_mount;
		unsigned int last_written;
		unsigned short mounts_since_check; // fsck
		unsigned short mounts_allowed_before_check;
		unsigned short signature;
		unsigned short state;
		unsigned short error_action;
		unsigned short minor_ver;
		unsigned int last_check;
		unsigned int forced_check_interval;
		unsigned int operating_system;
		unsigned int major_ver;
		unsigned short reserved_uid;
		unsigned short reserved_gid;
	};
	unsigned char buf[1024];
};
