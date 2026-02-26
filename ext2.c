#include "ext2.h"
#include "drivers/ide.h"
#include "libk/libk.h"
#include "tty/tty.h"
#include "errno.h"

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
	unsigned int singly_indirect_bp;
	unsigned int doubly_indirect_bp;
	unsigned int triply_indirect_bp;
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
	printk("ext2 header: version %u.%u, SB block %u, total inodes %u, total blocks %u, blocks reserved for SU %u, last mount time %u, block size %u bytes, blocks in group %u, required features %u, ro features %u, optional features %u, inode size %u, inodes in group %u\n",
			sb->major_ver, sb->minor_ver,
			sb->starting_block, sb->total_inodes,
			sb->total_blocks, sb->reserved_blocks, sb->last_mount, sb->block_size, sb->blocks_in_group,
			sb->required_features, sb->ro_features, sb->optional_features, sb->inode_size,
			sb->inodes_in_group);
}

static void ext2_read_block(struct ext2_fs *fs, unsigned char *buffer, unsigned int block) {
	for (unsigned int i = 0, j = 0; i < fs->sb.block_size; i+= 512, j += 1) {
		unsigned int sector = fs->partition.first_sector + block_to_lba(block, fs->sb.block_size) + j;
		//printk("reading sector %u to %p\n", sector, buffer + i);
		ide_read_sector(&fs->partition.drive, sector, buffer + i);
	}
}

/*static void print_block_group_descriptor(struct ext2_block_group_descriptor *descriptor) {
	printk("block usage bitmap BA %x, inode usage bitmap BA %x, start inode table BA %x, unallocated blocks %u, unallocated inodes %u, directory count %u\n", descriptor->ba_block_usage_bitmap, descriptor->ba_inode_usage_bitmap, descriptor->ba_start_inode_table, descriptor->unallocated_blocks, descriptor->unallocated_inodes, descriptor->directory_count);
}*/

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

static bool ext2_read_from_indirect_block(struct VecU8 *out, struct ext2_fs *fs, struct ext2_inode_extended *inode, unsigned int bp, unsigned int level) {
	union {
		unsigned char buf[4096];
		unsigned int block_pointers[1024];
	} indirect;
	ext2_read_block(fs, indirect.buf, bp);
	for (unsigned short i = 0; i < fs->sb.block_size / 4 && out->length < inode->base.size; i++) {
		if (level == 0) {
			if (!VecU8_reserve(out, fs->sb.block_size)) {
				VecU8_destruct(out);
				out->length = 0;
				return false;
			}
			ext2_read_block(fs, &out->data[out->length], indirect.block_pointers[i]);
			out->length += (out->length + fs->sb.block_size > inode->base.size) ? (inode->base.size - out->length) : fs->sb.block_size;
		} else if (!ext2_read_from_indirect_block(out, fs, inode, indirect.block_pointers[i], level - 1)) {
			return false;
		}
	}
	return true;
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
			return out;
		}
		ext2_read_block(fs, &out.data[out.length], inode->base.block_pointers[i]);
		out.length += (out.length + fs->sb.block_size > inode->base.size) ? (inode->base.size - out.length) : fs->sb.block_size;
	}
	if (out.length != inode->base.size) {
		if (!ext2_read_from_indirect_block(&out, fs, inode, inode->base.singly_indirect_bp, 0))
			return out;
		if (out.length != inode->base.size) {
			if (!ext2_read_from_indirect_block(&out, fs, inode, inode->base.doubly_indirect_bp, 1))
				return out;
			if (out.length != inode->base.size) {
				if (!ext2_read_from_indirect_block(&out, fs, inode, inode->base.triply_indirect_bp, 2))
					return out;
				if (out.length != inode->base.size)
					kernel_panic("ext2: not enough blocks available for inode size???", 0);
			}
		}
	}
	return out;
}

static bool ext2_get_inode(struct ext2_fs *fs, unsigned int inode, struct ext2_inode_extended *out) {
	unsigned int group = (inode - 1) / fs->sb.inodes_in_group;
	unsigned int index_in_group = (inode - 1) % fs->sb.inodes_in_group;
	unsigned int containing_inode_table_block = (index_in_group * fs->sb.inode_size) / fs->sb.block_size;
	unsigned int inode_table_block_index = index_in_group % (fs->sb.block_size / fs->sb.inode_size);
	//printk("inode %u's structure is in entry #%u of table block #%u in block group #%u\n", inode, table_block_index, containing_block, group);

	unsigned char group_desc_block[4096];

	// when blocks are 1024 octets, padding will be block 0, super block block 1 (just fits) and group desc table starts at block 2
	// otherwise, padding and SB are in one block and group desc table starts at block 1
	ext2_read_block(fs, group_desc_block, group * fs->sb.blocks_in_group + (fs->sb.block_size == 1024 ? 2 : 1));
	// TODO: read descriptor of the proper group
	struct ext2_block_group_descriptor *descriptor = (struct ext2_block_group_descriptor *)group_desc_block;
	//print_block_group_descriptor(descriptor);

	unsigned char block_inode_bitmap[4096];
	if (fs->sb.inodes_in_group > 32768) {
		kernel_panic("inodes_in_group > 32768", 0);
	}
	ext2_read_block(fs, block_inode_bitmap, descriptor->ba_inode_usage_bitmap);
	if ((block_inode_bitmap[index_in_group / 8] & (1 << ((inode - 1) % 8))) == 0) {
		print_err("tried to read non-allocated inode %u\n", inode);
		return false;
	}

	unsigned char block_inode_table[4096];
	ext2_read_block(fs, block_inode_table, descriptor->ba_start_inode_table + containing_inode_table_block);
	*out = ((struct ext2_inode_extended *)block_inode_table)[inode_table_block_index];
	return true;
}

/*static void print_inode(struct ext2_inode_extended *inode) {
	unsigned char modestr[11];
	format_mode(modestr, inode->base.mode);
	printk("%s %u %u:%u %u\n", modestr, inode->base.hard_link_count, inode->base.uid, inode->base.gid, inode->base.size);
}*/

static unsigned int ext2_path_to_inode(struct ext2_fs *fs, const char *path, unsigned int inode_nr) {
	const char *component_start = path;
	if (*path == '/') {
		component_start++;
		inode_nr = EXT2_ROOT_INODE;
	}
	const char *current_char;
	struct ext2_inode_extended current_inode;

	do {
		if (!ext2_get_inode(fs, inode_nr, &current_inode))
			return 0;
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
				while ((unsigned char *)direntry < file_contents.data + file_contents.length && direntry->inode) {
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
				} else
					return 0;
			}
		} else
			break;
		component_start = current_char;
	} while (1);

	return inode_nr;
}

unsigned int path_to_inode(const char *path, unsigned int relative_dir_inode_nr) {
	if (!ext2_mounted.mounted) return 0;
	unsigned int inode_nr = ext2_path_to_inode(&ext2_mounted, path, relative_dir_inode_nr);
	return inode_nr;
}

int unlink(const char *path, unsigned int relative_dir_inode_nr) {
	if (!ext2_mounted.mounted) return 0;
	unsigned int inode_nr = ext2_path_to_inode(&ext2_mounted, path, relative_dir_inode_nr);
	struct ext2_inode_extended inode;
	if (!ext2_get_inode(&ext2_mounted, inode_nr, &inode))
		return -EBADF;
	printk("links: %u\n", inode.base.hard_link_count);
	return 0;
}

bool stat_inode(unsigned int inode_nr, struct stat *out) {
	struct ext2_inode_extended inode;

	if (!ext2_get_inode(&ext2_mounted, inode_nr, &inode)) return -EBADF;
	out->st_dev = 0;
	out->st_ino = inode_nr;
	out->st_mode = inode.base.mode;
	out->st_nlink = inode.base.hard_link_count;
	out->st_uid = inode.base.uid;
	out->st_gid = inode.base.gid;
	out->st_size = inode.base.size;
	out->st_blksize = ext2_mounted.sb.block_size;
	out->st_blocks = ((inode.base.size + ext2_mounted.sb.block_size - 1) & ~(ext2_mounted.sb.block_size - 1)) / 512;
	out->st_mtim = (struct timespec){.tv_sec = inode.base.modification_time};
	out->st_atim = (struct timespec){.tv_sec = inode.base.access_time};
	out->st_ctim = (struct timespec){.tv_sec = inode.base.creation_time};
	return 0;
}

int read_inode(unsigned int inode_nr, unsigned int offset, char *buf, unsigned int buf_size) {
	if (!ext2_mounted.mounted) return -EBADF;

	struct ext2_inode_extended inode;

	if (!ext2_get_inode(&ext2_mounted, inode_nr, &inode)) return -EBADF;

	// someone won't be happy about this (hi thrieg)
	struct VecU8 contents = ext2_read_inode(&ext2_mounted, &inode);
	unsigned written_count = 0;
	for (unsigned int i = offset; i < contents.length && written_count < buf_size; i++)
		buf[written_count++] = contents.data[i];

	VecU8_destruct(&contents);

	return written_count;
}

enum ext2_direnttype {
	EXT2_UNKNOWN,
	EXT2_REGULAR,
	EXT2_DIR,
	EXT2_CHAR,
	EXT2_BLOCK,
	EXT2_FIFO,
	EXT2_SOCKET,
	EXT2_SYMLINK
};

enum norm_direnttype {
	DT_UNKNOWN = 0,
	DT_FIFO = 1,
	DT_CHR = 2,
	DT_DIR = 4,
	DT_BLK = 6,
	DT_REG = 8,
	DT_LNK = 10,
	DT_SOCK = 12,
	DT_WHT = 14
};

static enum norm_direnttype ext2_direnttype_to_norm(enum ext2_direnttype type) {
	switch (type) {
		case EXT2_REGULAR: return DT_REG;
		case EXT2_DIR: return DT_DIR;
		case EXT2_CHAR: return DT_CHR;
		case EXT2_BLOCK: return DT_BLK;
		case EXT2_FIFO: return DT_FIFO;
		case EXT2_SOCKET: return DT_SOCK;
		case EXT2_SYMLINK: return DT_LNK;
		default: return DT_UNKNOWN;
	}
}

int getdents(unsigned int inode_nr, struct linux_dirent64 *ent, unsigned int count, unsigned int offset) {
	if (!ext2_mounted.mounted) return -EBADF;

	struct ext2_inode_extended inode;

	if (!ext2_get_inode(&ext2_mounted, inode_nr, &inode)) return -EBADF;

	// someone won't be happy about this (hi thrieg)
	struct VecU8 contents = ext2_read_inode(&ext2_mounted, &inode);
	unsigned written_count = 0;

	struct ext2_direntry *direntry = (struct ext2_direntry *)(contents.data + offset);
	while ((unsigned char *)direntry < contents.data + contents.length && direntry->inode) {
		unsigned int reclen = sizeof(struct linux_dirent64) + direntry->name_length + 1;
		//printk("%u", (unsigned int)sizeof(struct linux_dirent64));
		reclen = (reclen + 7) & (~7);
		if (written_count + reclen > count)
			break;
		ent->d_ino = direntry->inode;
		ent->d_off = 0;
		ent->d_type = ext2_direnttype_to_norm(direntry->type_indicator);
		ent->d_reclen = reclen;
		memcpy(ent->d_name, direntry->name, direntry->name_length);
		ent->d_name[direntry->name_length] = '\0';
		written_count += reclen;

		direntry = (struct ext2_direntry *)((unsigned char *)direntry + direntry->entry_size);
		ent = (struct linux_dirent64 *)((unsigned char *)ent + reclen);
	}

	VecU8_destruct(&contents);

	return written_count;
}

// TODO: this currently writes path in reverse order (better than nothing I guess)
int getdirname(unsigned int inode_nr, char *buf, unsigned int size) {
	if (!ext2_mounted.mounted) return -EBADF;
	if (size == 0)
		return -ERANGE;

	struct ext2_inode_extended inode;

	if (!ext2_get_inode(&ext2_mounted, inode_nr, &inode)) return -EBADF;
	*buf = '/';

	unsigned written_count = 1;
	while (1) {
		if ((inode.base.mode >> 12) != MODE_DIRECTORY)
			return -ENOTDIR;

		unsigned int parent_inode_nr = ext2_path_to_inode(&ext2_mounted, "..", inode_nr);
		if (parent_inode_nr == 0) return -EBADF;
		if (parent_inode_nr == inode_nr) {
			if (size == written_count)
				return -ERANGE;
			buf[written_count++] = '\0';
			return written_count; // root reached
		}

		if (!ext2_get_inode(&ext2_mounted, parent_inode_nr, &inode)) return -EBADF;

		struct VecU8 contents = ext2_read_inode(&ext2_mounted, &inode);

		struct ext2_direntry *direntry = (struct ext2_direntry *)(contents.data);
		while ((unsigned char *)direntry < contents.data + contents.length && direntry->inode) {
			if (direntry->inode == inode_nr) {
				if (written_count + direntry->name_length > size) {
					VecU8_destruct(&contents);
					return -ERANGE;
				}
				memcpy(buf + written_count, direntry->name, direntry->name_length);
				written_count += direntry->name_length;
			}

			direntry = (struct ext2_direntry *)((unsigned char *)direntry + direntry->entry_size);
		}
		VecU8_destruct(&contents);
		inode_nr = parent_inode_nr;
	}
}

struct VecU8 read_full_file(const char *path) {
	struct VecU8 empty = {.length = 0};
	if (!ext2_mounted.mounted) return empty;

	unsigned int inode_nr = ext2_path_to_inode(&ext2_mounted, path, EXT2_ROOT_INODE);
	if (!inode_nr) return empty;

	struct ext2_inode_extended inode;
	if (!ext2_get_inode(&ext2_mounted, inode_nr, &inode)) return empty;

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
	if (sb.major_ver != 1 || sb.block_size > 4096)
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

	struct ext2_inode_extended root_dir;

	if (!ext2_get_inode(&ext2_mounted, EXT2_ROOT_INODE, &root_dir)) {
		kernel_panic("couldn't retrieve root inode from ext2 fs", 0);
	}
	struct VecU8 file_contents = ext2_read_inode(&ext2_mounted, &root_dir);

	struct ext2_direntry *direntry = (struct ext2_direntry *)file_contents.data;
	while ((unsigned char *)direntry < file_contents.data + file_contents.length && direntry->inode) {
		printk("%u %u - ", direntry->type_indicator, direntry->inode);
		write((const char *)direntry->name,  direntry->name_length);
		writes("\n");
		direntry = (struct ext2_direntry *)((unsigned char *)direntry + direntry->entry_size);
	}
	printk("'/bin/init' is inode %u\n", ext2_path_to_inode(&ext2_mounted, "/bin/init", EXT2_ROOT_INODE));
	printk("'//////bin/init' is inode %u\n", ext2_path_to_inode(&ext2_mounted, "//////bin/init", EXT2_ROOT_INODE));
	printk("'//////bin////init' is inode %u\n", ext2_path_to_inode(&ext2_mounted, "//////bin////init", EXT2_ROOT_INODE));
	printk("'//////bin////init/' is inode %u\n", ext2_path_to_inode(&ext2_mounted, "//////bin////init/", EXT2_ROOT_INODE));
	printk("'/' is inode %u\n", ext2_path_to_inode(&ext2_mounted, "/", EXT2_ROOT_INODE));
	VecU8_destruct(&file_contents);
}
