#include "ext2.h"
#include "drivers/ide.h"
#include "libk/libk.h"
#include "tty/tty.h"

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

static struct ext2_fs {
	union ext2_super_block sb;
	struct ide_partition partition;
	bool mounted;
} ext2_mounted;

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

static void ext2_read_block(struct ext2_fs *fs, unsigned char *buffer, unsigned int block) {
	for (unsigned int i = 0, j = 0; i < fs->sb.block_size; i+= 512, j += 1) {
		unsigned int sector = fs->partition.first_sector + block_to_lba(block, fs->sb.block_size) + j;
		//printk("reading sector %u to %p\n", sector, buffer + i);
		ide_read_sector(&fs->partition.drive, sector, buffer + i);
	}
}

#define ROOT_INODE 2

/*static void print_block_group_descriptor(struct ext2_block_group_descriptor *descriptor) {
	printk("block usage bitmap BA %x, inode usage bitmap BA %x, start inode table BA %x, unallocated blocks %u, unallocated inodes %u, directory count %u\n", descriptor->ba_block_usage_bitmap, descriptor->ba_inode_usage_bitmap, descriptor->ba_start_inode_table, descriptor->unallocated_blocks, descriptor->unallocated_inodes, descriptor->directory_count);
}*/

#define MODE_FIFO 0x1
#define MODE_CHAR 0x2
#define MODE_DIRECTORY 0x4
#define MODE_BLOCKDEV 0x6
#define MODE_REGULAR 0x8
#define MODE_SYMLINK 0xA
#define MODE_SOCKET 0xC

void fs_format_mode(unsigned char out[11], unsigned short mode) {
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

static struct VecU8 ext2_read_inode(struct ext2_fs *fs, struct ext2_inode_extended *inode) {
	struct VecU8 out;
	VecU8_init(&out);
	VecU8_reserve(&out, inode->base.size);
	//printk("reading inode contents of size %u\n", inode->base.size);

	for (unsigned short i = 0; i < sizeof(inode->base.block_pointers)/sizeof(inode->base.block_pointers[0]) && out.length < inode->base.size; i++) {
		if (!VecU8_reserve(&out, fs->sb.block_size)) {
			VecU8_destruct(&out);
			out.length = 0;
		}
		ext2_read_block(fs, &out.data[out.length], inode->base.block_pointers[i]);
		out.length += (out.length + fs->sb.block_size > inode->base.size) ? (inode->base.size - out.length - fs->sb.block_size) : fs->sb.block_size;
	}
	if (out.length != inode->base.size) {
		kernel_panic("direct block pointers are not sufficient for inode", 0);
	}
	return out;
}

static struct ext2_inode_extended ext2_get_inode(struct ext2_fs *fs, unsigned int inode) {
	unsigned int group = (inode - 1) / fs->sb.inodes_in_group;
	unsigned int group_index = (inode - 1) % fs->sb.inodes_in_group;
	unsigned int containing_block = (group_index * fs->sb.inode_size) / fs->sb.block_size;
	unsigned int table_block_index = group_index % (fs->sb.block_size / fs->sb.inode_size);
	//printk("inode %u's structure is in entry #%u of table block #%u in block group #%u\n", inode, table_block_index, containing_block, group);

	unsigned char group_desc_block[4096];
	ext2_read_block(fs, group_desc_block, group * fs->sb.blocks_in_group + 1);
	struct ext2_block_group_descriptor *descriptor = (struct ext2_block_group_descriptor *)group_desc_block;
	//print_block_group_descriptor(descriptor);

	/*unsigned char block_inode_bitmap[4096];
	ext2_read_block(sb, partition, block_inode_bitmap, descriptor->ba_inode_usage_bitmap);*/
	// TODO: check if inode is present in bitmap before reading struct or is it useless?

	unsigned char block_inode_table[4096];
	ext2_read_block(fs, block_inode_table, descriptor->ba_start_inode_table + containing_block);
	struct ext2_inode_extended inode_struct = ((struct ext2_inode_extended *)block_inode_table)[table_block_index];
	return inode_struct;
}

/*static void print_inode(struct ext2_inode_extended *inode) {
	unsigned char modestr[11];
	format_mode(modestr, inode->base.mode);
	printk("%s %u %u:%u %u\n", modestr, inode->base.hard_link_count, inode->base.uid, inode->base.gid, inode->base.size);
}*/

static unsigned int ext2_find_absolute(struct ext2_fs *fs, const char *path) {
	if (*path != '/')
		return 0;
	const char *current_char;
	const char *component_start = path + 1;
	unsigned int inode_nr = ROOT_INODE;
	struct ext2_inode_extended current_inode = ext2_get_inode(fs, ROOT_INODE);

	do {
		//print_inode(&current_inode);
		while (*component_start == '/') {
			component_start++;
		}
		current_char = component_start;
		while (*current_char && *current_char != '/') {
			current_char++;
		}
		unsigned int component_length = current_char - component_start;
		if (component_length > 0) {
			if ((current_inode.base.mode >> 12) != MODE_DIRECTORY)
				return 0;
			else {
				// lookup directory entry
				struct VecU8 file_contents = ext2_read_inode(fs, &current_inode);

				unsigned int found_inode_nr = 0;
				struct ext2_direntry *direntry = (struct ext2_direntry *)file_contents.data;
				while (direntry->inode) {
					//printk("%u %u - ", direntry->type_indicator, direntry->inode);
					//write((const char *)direntry->name,  direntry->name_length);
					//writes("\n");
					if (direntry->name_length == component_length && memcmp(direntry->name, component_start, component_length) == 0) {
						found_inode_nr = direntry->inode;
						break;
					}
					direntry = (struct ext2_direntry *)((unsigned char *)direntry + direntry->entry_size);
				}
				VecU8_destruct(&file_contents);
				if (found_inode_nr) {
					inode_nr = found_inode_nr;
					current_inode = ext2_get_inode(fs, found_inode_nr);
				} else
					return 0;
			}
		} else
			break;
		component_start = current_char;
	} while (1);

	return inode_nr;
}

struct VecU8 read_full_file(const char *path) {
	struct VecU8 empty = {.length = 0};
	if (!ext2_mounted.mounted) return empty;

	unsigned int inode_nr = ext2_find_absolute(&ext2_mounted, path);
	if (!inode_nr) return empty;

	struct ext2_inode_extended inode = ext2_get_inode(&ext2_mounted, inode_nr);

	return ext2_read_inode(&ext2_mounted, &inode);
}

/* Called during PCI IDE initialization */
void ext2_test(struct ide_partition *partition) {
	if (partition->sector_count < 3)
		return;
	// First 1024 bytes (two sectors) in partition are ignored
	printk("ext2 partition first sector: %u\n", partition->first_sector);
	union ext2_super_block sb;
	ide_read_sector(&partition->drive, partition->first_sector + 2, sb.buf);
	ide_read_sector(&partition->drive, partition->first_sector + 3, sb.buf + 512);

	// I believe blocks can't be larger than 2^32
	sb.block_size = 1024 << sb.block_size;

	ext2_print_sb(&sb);
	if (sb.major_ver != 1 || sb.block_size != 4096)
		return;

	// Only the last discovered ext2 partition will be available.
	ext2_mounted.sb = sb;
	ext2_mounted.partition = *partition;
	ext2_mounted.mounted = true;

	// verify that block group 2 has same super block
	/*union ext2_super_block sb2;
	unsigned int sector_sb2 = lba_start + block_to_lba(sb.blocks_in_group, bytes_per_block);
	printk("sector_sb2: %u %u %u\n", sector_sb2);
	ide_read_sector(drive, sector_sb2, sb2.buf);
	ide_read_sector(drive, sector_sb2 + 1, sb2.buf + 512);
	ext2_print_sb(&sb2);*/

	struct ext2_inode_extended root_dir = ext2_get_inode(&ext2_mounted, ROOT_INODE);
	struct VecU8 file_contents = ext2_read_inode(&ext2_mounted, &root_dir);

	struct ext2_direntry *direntry = (struct ext2_direntry *)file_contents.data;
	while (direntry->inode) {
		printk("%u %u - ", direntry->type_indicator, direntry->inode);
		write((const char *)direntry->name,  direntry->name_length);
		writes("\n");
		direntry = (struct ext2_direntry *)((unsigned char *)direntry + direntry->entry_size);
	}
	printk("'/bin/init' is inode %u\n", ext2_find_absolute(&ext2_mounted, "/bin/init"));
	printk("'//////bin/init' is inode %u\n", ext2_find_absolute(&ext2_mounted, "//////bin/init"));
	printk("'//////bin////init' is inode %u\n", ext2_find_absolute(&ext2_mounted, "//////bin////init"));
	printk("'//////bin////init/' is inode %u\n", ext2_find_absolute(&ext2_mounted, "//////bin////init/"));
	printk("'/' is inode %u\n", ext2_find_absolute(&ext2_mounted, "/"));
	VecU8_destruct(&file_contents);
}
