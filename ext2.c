#include "ext2.h"
#include "drivers/ide.h"
#include "libk/libk.h"

struct ext2_block_group_descriptor {
	// ba = address of block
	unsigned int ba_block_usage_bitmap;
	unsigned int ba_inode_usage_bitmap;
	unsigned int ba_start_inode_table;
	unsigned short unallocated_blocks;
	unsigned short unallocated_inodes;
	unsigned short directory_count;
	unsigned char _unused[14];
} __attribute__((packed));

struct ext2_inode {
	unsigned short mode;
	unsigned short uid;
	unsigned int size;
	unsigned int access_time;
	unsigned int creation_time;
	unsigned int modification_time;
	unsigned int deletion_time;
	unsigned short gid;
	unsigned short hard_link_count;
	unsigned int disk_sector_count;
	unsigned int flags;
	unsigned int operating_system_specific_1;
	unsigned int block_pointers[12];
	unsigned int singly_indirected_bp;
	unsigned int doubly_indirected_bp;
	unsigned int triply_indirected_bp;
	unsigned int generation_number;
	unsigned int extended_attribute_block;
	unsigned int size_upper;
	unsigned int fragment_block_address;
	unsigned char operating_system_specific_2[12];
};

static unsigned int block_to_lba(unsigned int block, unsigned int block_size) {
	return block * (block_size / 512);
}

static void ext2_print_sb(union ext2_super_block *sb) {
	printk("ext2 header: SB block %u, total inodes %u, total blocks %u, blocks reserved for SU %u, last mount time %u, block size %u bytes, blocks in group %u\n", sb->starting_block, sb->total_inodes,
			sb->total_blocks, sb->reserved_blocks, sb->last_mount, 1024 << sb->block_size, sb->blocks_in_group);
}

void ext_read_block(struct ide_drive *drive, unsigned int lba_start, unsigned char *buffer, unsigned int block, unsigned int block_size) {
	for (unsigned int i = 0, j = 0; i < block_size; i+= 512, j += 1) {
		unsigned int sector = lba_start + block_to_lba(block, block_size) + j;
		//printk("reading sector %u to %p\n", sector, buffer + i);
		ide_read_sector(drive, sector, buffer + i);
	}
}

#define INODE_SIZE 128

static void print_block_group_descriptor(struct ext2_block_group_descriptor *descriptor) {
	printk("block usage bitmap BA %x, inode usage bitmap BA %x, start inode table BA %x, unallocated blocks %u, unallocated inodes %u, directory count %u\n", descriptor->ba_block_usage_bitmap, descriptor->ba_inode_usage_bitmap, descriptor->ba_start_inode_table, descriptor->unallocated_blocks, descriptor->unallocated_inodes, descriptor->directory_count);
}

#define MODE_FIFO 0x1
#define MODE_CHAR 0x2
#define MODE_DIRECTORY 0x4
#define MODE_BLOCKDEV 0x6
#define MODE_REGULAR 0x8
#define MODE_SYMLINK 0xA
#define MODE_SOCKET 0xC

void format_mode(unsigned char out[11], unsigned short mode) {
	switch (mode >> 12) {
		case MODE_FIFO:
			out[0] = 'f'; break;
		case MODE_CHAR:
			out[0] = 'c'; break;
		case MODE_DIRECTORY:
			out[0] = 'd'; break;
		case MODE_BLOCKDEV:
			out[0] = 'b'; break;
		case MODE_REGULAR:
			out[0] = '-'; break;
		case MODE_SYMLINK:
			out[0] = 'l'; break;
		case MODE_SOCKET:
			out[0] = 's'; break;
		default:
			out[0] = '?';
	}
	out[1] = mode & 0400 ? 'r' : '-';
	out[2] = mode & 0200 ? 'w' : '-';
	out[3] = mode & 0100 ? 'x' : '-';
	out[4] = mode & 0040 ? 'r' : '-';
	out[5] = mode & 0020 ? 'w' : '-';
	out[6] = mode & 0010 ? 'x' : '-';
	out[7] = mode & 0004 ? 'r' : '-';
	out[8] = mode & 0002 ? 'w' : '-';
	out[9] = mode & 0001 ? 'x' : '-';
	out[10] = 0;
}

static void print_inode(struct ext2_inode *inode) {
	unsigned char modestr[11];
	format_mode(modestr, inode->mode);
	printk("%s %u %u:%u\n", modestr, inode->hard_link_count, inode->uid, inode->gid);
}

void ext2(struct ide_drive *drive, unsigned int lba_start, unsigned int total_sectors) {
	if (total_sectors < 3)
		return;
	// First 1024 bytes (two sectors) in partition are ignored
	printk("SB sector: %u\n", lba_start);
	union ext2_super_block sb;
	ide_read_sector(drive, lba_start + 2, sb.buf);
	ide_read_sector(drive, lba_start + 3, sb.buf + 512);

	ext2_print_sb(&sb);
	const unsigned int bytes_per_block = 1024 << sb.block_size;
	if (bytes_per_block != 4096)
		return;
	const unsigned int root_dir_inode = 2;
	unsigned int root_dir_group = (root_dir_inode - 1) / sb.inodes_in_group;
	unsigned int group_index = (root_dir_inode - 1) / sb.inodes_in_group;
	unsigned int containing_block = group_index * INODE_SIZE / bytes_per_block;
	printk("root dir group %u containing block %u\n", root_dir_group, containing_block);

	// verify that block group 2 has same super block
	/*union ext2_super_block sb2;
	unsigned int sector_sb2 = lba_start + block_to_lba(sb.blocks_in_group, bytes_per_block);
	printk("sector_sb2: %u %u %u\n", sector_sb2);
	ide_read_sector(drive, sector_sb2, sb2.buf);
	ide_read_sector(drive, sector_sb2 + 1, sb2.buf + 512);
	ext2_print_sb(&sb2);*/

	unsigned char block[4096];
	ext_read_block(drive, lba_start, block, 1, bytes_per_block);

	struct ext2_block_group_descriptor *descriptor = (struct ext2_block_group_descriptor *)block;
	print_block_group_descriptor(descriptor);

	ext_read_block(drive, lba_start, block, descriptor->ba_start_inode_table, bytes_per_block);
	struct ext2_inode *inode = (struct ext2_inode *)block;
	for (unsigned short i = 0; i < 16; i++)
		print_inode(&inode[i]);
}

