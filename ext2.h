#include "drivers/ide.h"

void ext2_test(struct ide_partition *partition);

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
		// only present if major_ver >= 1
		unsigned int first_non_reserved_inode;
		unsigned short inode_size;
		unsigned short this_block_group; // Block group that this SB is part of (backups)
		unsigned int optional_features;
		unsigned int required_features;
		unsigned int ro_features;
		unsigned char fs_id[16];
		unsigned char volume_name[16];
		unsigned char last_mount_point[64];
		unsigned int compression_algorithms;
		unsigned char preallocate_blocks_files;
		unsigned char preallocate_blocks_dirs;
		unsigned short _unused;
		unsigned char journal_id[16];
		unsigned int journal_inode;
		unsigned int journal_device;
		unsigned int orphan_inode_head;
	};
	unsigned char buf[1024];
} __attribute__((packed));
