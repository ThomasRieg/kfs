#include "ext2.h"
#include "drivers/ide.h"
#include "libk/libk.h"
#include "tty/tty.h"

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

#define INODE_SIZE_BEFORE_MAJ_1 128

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
} __attribute__((packed));

struct ext2_inode_extended {
	struct ext2_inode base;
	unsigned char _unused[128];
} __attribute__((packed));

_Static_assert(sizeof(struct ext2_inode) == INODE_SIZE_BEFORE_MAJ_1, "struct ext2_inode has incorrect size");
_Static_assert(sizeof(struct ext2_inode_extended) == 256, "struct ext2_inode_extended has incorrect size");

struct ext2_direntry {
	unsigned int inode;
	unsigned short entry_size;
	unsigned char name_length;
	unsigned char type_indicator;
	unsigned char name[];
} __attribute__((packed));

static unsigned int block_to_lba(unsigned int block, unsigned int block_size) {
	return block * (block_size / 512);
}

static void ext2_print_sb(union ext2_super_block *sb) {
	printk("ext2 header: version %u.%u, SB block %u, total inodes %u, total blocks %u, blocks reserved for SU %u, last mount time %u, block size %u bytes, blocks in group %u, required features %u, ro features %u, optional features %u, inode size %u\n",
			sb->major_ver, sb->minor_ver,
			sb->starting_block, sb->total_inodes,
			sb->total_blocks, sb->reserved_blocks, sb->last_mount, sb->block_size, sb->blocks_in_group,
			sb->required_features, sb->ro_features, sb->optional_features, sb->inode_size);
}

void ext2_read_block(union ext2_super_block *sb, struct ide_partition *partition, unsigned char *buffer, unsigned int block) {
	for (unsigned int i = 0, j = 0; i < sb->block_size; i+= 512, j += 1) {
		unsigned int sector = partition->first_sector + block_to_lba(block, sb->block_size) + j;
		//printk("reading sector %u to %p\n", sector, buffer + i);
		ide_read_sector(partition->drive, sector, buffer + i);
	}
}

#define ROOT_INODE 2

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

struct VecU8 ext2_read_inode(union ext2_super_block *sb, struct ide_partition *partition, struct ext2_inode_extended *inode) {
	struct VecU8 out;
	VecU8_init(&out);

	printk("%u\n", inode->base.size);

	for (unsigned short i = 0; i < sizeof(inode->base.block_pointers)/sizeof(inode->base.block_pointers[0]) && out.length < inode->base.size; i++) {
		if (!VecU8_reserve(&out, sb->block_size)) return out;
		ext2_read_block(sb, partition, &out.data[out.length], inode->base.block_pointers[i]);
		out.length += (out.length + sb->block_size > inode->base.size) ? (inode->base.size - out.length - sb->block_size) : sb->block_size;
	}
	if (out.length != inode->base.size) {
		kernel_panic("direct block pointers are not sufficient for inode", 0);
	}
	return out;
}

static void print_inode(struct ext2_inode_extended *inode) {
	unsigned char modestr[11];
	format_mode(modestr, inode->base.mode);
	printk("%s %u %u:%u %u\n", modestr, inode->base.hard_link_count, inode->base.uid, inode->base.gid, inode->base.size);
}

void ext2_test(struct ide_partition *partition) {
	if (partition->sector_count < 3)
		return;
	// First 1024 bytes (two sectors) in partition are ignored
	printk("ext2 partition first sector: %u\n", partition->first_sector);
	union ext2_super_block sb;
	ide_read_sector(partition->drive, partition->first_sector + 2, sb.buf);
	ide_read_sector(partition->drive, partition->first_sector + 3, sb.buf + 512);

	// I believe blocks can't be larger than 2^32
	sb.block_size = 1024 << sb.block_size;

	ext2_print_sb(&sb);
	if (sb.major_ver != 1 || sb.block_size != 4096)
		return;
	unsigned int root_dir_group = (ROOT_INODE - 1) / sb.inodes_in_group;
	unsigned int group_index = (ROOT_INODE - 1) / sb.inodes_in_group;
	unsigned int containing_block = group_index * sb.inode_size / sb.block_size;
	printk("root dir group %u containing block %u\n", root_dir_group, containing_block);

	// verify that block group 2 has same super block
	/*union ext2_super_block sb2;
	unsigned int sector_sb2 = lba_start + block_to_lba(sb.blocks_in_group, bytes_per_block);
	printk("sector_sb2: %u %u %u\n", sector_sb2);
	ide_read_sector(drive, sector_sb2, sb2.buf);
	ide_read_sector(drive, sector_sb2 + 1, sb2.buf + 512);
	ext2_print_sb(&sb2);*/

	unsigned char block[4096];
	ext2_read_block(&sb, partition, block, 1);

	struct ext2_block_group_descriptor *descriptor = (struct ext2_block_group_descriptor *)block;
	print_block_group_descriptor(descriptor);

	unsigned char block_inode_bitmap[4096];
	ext2_read_block(&sb, partition, block_inode_bitmap, descriptor->ba_inode_usage_bitmap);

	unsigned char block_inode_table[4096];
	ext2_read_block(&sb, partition, block_inode_table, descriptor->ba_start_inode_table);
	struct ext2_inode_extended *inode = (struct ext2_inode_extended *)block_inode_table;

	for (unsigned short i = 0; i < 16; i++)
		print_inode(&inode[i]);
	struct VecU8 file_contents = ext2_read_inode(&sb, partition, inode + ROOT_INODE - 1);

	struct ext2_direntry *direntry = (struct ext2_direntry *)file_contents.data;
	while (direntry->inode) {
		printk("%u %u - ", direntry->type_indicator, direntry->inode);
		write((const char *)direntry->name,  direntry->name_length);
		writes("\n");
		direntry = (struct ext2_direntry *)((unsigned char *)direntry + direntry->entry_size);
	}
	VecU8_destruct(&file_contents);
}
