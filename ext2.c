#include "ext2.h"
#include "drivers/ide.h"
#include "libk/libk.h"

struct block_group_descriptor {
	// ba = address of block
	unsigned int ba_block_usage_bitmap;
	unsigned int ba_inode_usage_bitmap;
	unsigned int ba_start_inode_table;
	unsigned short unallocated_blocks;
	unsigned short unallocated_inodes;
	unsigned short directory_count;
	unsigned char _unused[14];
} __attribute__((packed));

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
		printk("reading sector %u to %p\n", sector, buffer + i);
		ide_read_sector(drive, sector, buffer + i);
	}
}

#define INODE_SIZE 128

void ext2(struct ide_drive *drive, unsigned int lba_start, unsigned int total_sectors) {
	if (total_sectors < 3)
		return;
	// First 1024 bytes (two sectors) in partition are ignored
	lba_start += 2;
	printk("SB sector: %u\n", lba_start);
	union ext2_super_block sb;
	ide_read_sector(drive, lba_start, sb.buf);
	ide_read_sector(drive, lba_start + 1, sb.buf + 512);

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

	unsigned char block1[4096];
	ext_read_block(drive, lba_start, block1, 0, bytes_per_block);
	for (unsigned int i = 0; i < sizeof(block1); i += 32) {
		hex_dump(block1 + i, 32);
		printk("\n");
	}
}

