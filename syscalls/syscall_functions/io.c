#include "../syscalls.h"
#include "../../tasks/task.h"
#include "../../vmalloc/vmalloc.h"
#include "../../libk/libk.h"
#include "../../mmap/mmap.h"
#include "../../mem_page/mem_paging.h"
#include "../../errno.h"
#include "../../ext2.h"
#include "../../tty/tty.h"

#define AT_EMPTY_PATH 0x1000
#define AT_FDCWD -100

struct statx_timestamp {
	int64_t tv_sec;    /* Seconds since the Epoch (UNIX time) */
	unsigned int tv_nsec;   /* Nanoseconds since tv_sec */
};

struct statx {
	unsigned int stx_mask;        /* Mask of bits indicating
	filled fields */
	unsigned int stx_blksize;     /* Block size for filesystem I/O */
	uint64_t stx_attributes;  /* Extra file attribute indicators */
	unsigned int stx_nlink;       /* Number of hard links */
	unsigned int stx_uid;         /* User ID of owner */
	unsigned int stx_gid;         /* Group ID of owner */
	unsigned short stx_mode;        /* File type and mode */
	uint64_t stx_ino;         /* Inode number */
	uint64_t stx_size;        /* Total size in bytes */
	uint64_t stx_blocks;      /* Number of 512B blocks allocated */
	uint64_t stx_attributes_mask;
	/* Mask to show what's supported
	in stx_attributes */

	/* The following fields are file timestamps */
	struct statx_timestamp stx_atime;  /* Last access */
	struct statx_timestamp stx_btime;  /* Creation */
	struct statx_timestamp stx_ctime;  /* Last status change */
	struct statx_timestamp stx_mtime;  /* Last modification */

	/* If this file represents a device, then the next two
	fields contain the ID of the device */
	unsigned int stx_rdev_major;  /* Major ID */
	unsigned int stx_rdev_minor;  /* Minor ID */

	/* The next two fields contain the ID of the device
	containing the filesystem where the file resides */
	unsigned int stx_dev_major;   /* Major ID */
	unsigned int stx_dev_minor;   /* Minor ID */

	uint64_t stx_mnt_id;      /* Mount ID */

	/* Direct I/O alignment restrictions */
	unsigned int stx_dio_mem_align;
	unsigned int stx_dio_offset_align;

	uint64_t stx_subvol;      /* Subvolume identifier */

	/* Direct I/O atomic write limits */
	unsigned int stx_atomic_write_unit_min;
	unsigned int stx_atomic_write_unit_max;
	unsigned int stx_atomic_write_segments_max;

	/* File offset alignment for direct I/O reads */
	unsigned int stx_dio_read_offset_align;

	/* Direct I/O atomic write max opt limit */
	unsigned int stx_atomic_write_unit_max_opt;
};

static struct open_file *get_file_from_fd(int fd) {
	struct open_file *file = fd >= 0 && (unsigned int)fd < MAX_OPEN_FILES ? g_curr_task->open_files[fd] : 0;
	return file;
}

#define STATX_TYPE 0x00000001U
#define STATX_BASIC_STATS 0x000007ffU
#define STATX_MNT_ID 0x00001000U

uint32_t syscall_statx(t_interrupt_data *regs) {
	int dirfd = regs->ebx;
	const char *path = (char *)regs->ecx;
	unsigned int flags = regs->edx;
	unsigned int mask = regs->esi;
	struct statx *buffer = (struct statx *)regs->edi;
	printk("statx at eip %p: %u %s %u %u %p\n", regs->eip, dirfd, path, flags, mask, buffer);
	if (path[0] == 0) {
		if (!(flags & AT_EMPTY_PATH))
			return (-ENOENT);
		struct open_file *file = get_file_from_fd(dirfd);
		if (!file) return -EBADF;

		*buffer = (struct statx){
			.stx_mask = STATX_BASIC_STATS | STATX_MNT_ID,
			.stx_mode = (file->type == FILE_TERMINAL ? MODE_CHAR : MODE_REGULAR) << 12,
			.stx_ino = (file->type == FILE_REGULAR ? file->inode.inode_nr : 0 ),
		};
		if (file->type == FILE_REGULAR) {
			struct stat stat;
			int status = stat_inode(file->inode.inode_nr, &stat);
			if (status) {
				return status;
			} else {
				buffer->stx_size = stat.st_size;
				buffer->stx_mode = stat.st_mode;
			}
		}
		return 0;
	}
	memset(buffer, 0, sizeof(struct statx));
	return 0;
}

uint32_t syscall_fstatat(t_interrupt_data *regs) {
	int dirfd = regs->ebx;
	const char *path = (char *)regs->ecx;
	struct stat *buf = (struct stat *)regs->edx;
	unsigned int flags = regs->esi;
	printk("fstatat at eip %p: %u %s %p %u\n", regs->eip, dirfd, path, buf, flags);
	if (path[0] == 0 && !(flags & AT_EMPTY_PATH))
		return (-ENOENT);
	memset(buf, 0, sizeof (struct stat));
	return (0);
}

uint32_t syscall_readlink(t_interrupt_data *regs) {
	const char *path = (char *)regs->ebx;
	char *buf = (char *)regs->ecx;
	unsigned int buf_size = regs->edx;
	printk("readlink: %s %p %u\n", path, buf, buf_size);
	return (-ENOSYS);
}

uint32_t syscall_openat(t_interrupt_data *regs) {
	int dirfd = regs->ebx;
	const char *path = (char *)regs->ecx;
	int open_flag = regs->edx;
	printk("openat: %u %s %u\n", dirfd, path, open_flag);
	unsigned short i;
	for (i = 0; i < MAX_OPEN_FILES; i++) {
		if (!g_curr_task->open_files[i])
			break;
	}
	if (i >= MAX_OPEN_FILES)
		return (-EMFILE);
	if (strcmp(path, "/dev/tty1") == 0) {
		struct open_file *file = vmalloc(sizeof(struct open_file));
		if (!file)
			return (-ENOMEM);
		file->ref_count = 1;
		file->type = FILE_TERMINAL;
		g_curr_task->open_files[i] = file;
		return i;
	}
	unsigned int dir_inode;
	if (dirfd == AT_FDCWD)
		dir_inode = g_curr_task->cwd_inode_nr;
	else {
		struct open_file *file = get_file_from_fd(dirfd);
		if (file->type != FILE_REGULAR)
			return -EBADF;
		dir_inode = file->inode.inode_nr;
	}
	unsigned int inode_nr = path_to_inode(path, dir_inode);
	printk("openat inode_nr: %u\n", inode_nr);
	if (!inode_nr)
		return -ENOENT;

	struct open_file *file = vmalloc(sizeof(struct open_file));
	if (!file)
		return (-ENOMEM);
	file->ref_count = 1;
	file->type = FILE_REGULAR;
	file->inode.inode_nr = inode_nr;
	file->inode.file_offset = 0;
	file->inode.open_status = open_flag;

	g_curr_task->open_files[i] = file;
	return i;
}

uint32_t syscall_dup(t_interrupt_data *regs) {
	int fd = regs->ebx;
	printk("dup: %u\n", fd);
	struct open_file *file = get_file_from_fd(fd);
	if (!file)
		return (-EBADF);
	unsigned short i;
	for (i = 0; i < MAX_OPEN_FILES; i++) {
		if (!g_curr_task->open_files[i])
			break;
	}
	if (i >= MAX_OPEN_FILES)
		return (-EMFILE);
	g_curr_task->open_files[i] = file;
	file->ref_count += 1;
	return (i);
}

uint32_t syscall_close(t_interrupt_data *regs) {
	int fd = regs->ebx;
	printk("close: %u\n", fd);
	struct open_file *file = get_file_from_fd(fd);
	if (!file)
		return (-EBADF);
	file->ref_count -= 1;
	if (file->ref_count == 0)
		vfree(file);
	return (0);
}

uint32_t syscall_fcntl64(t_interrupt_data *regs) {
	int fd = regs->ebx;
	int cmd = regs->ecx;
	printk("fcntl64: %u %u\n", fd, cmd);
	return 0;
}

uint32_t syscall_read(t_interrupt_data *regs)
{
	unsigned int fd = regs->ebx;
	char *buf = (char *)regs->ecx;
	unsigned int count = regs->edx;
	printk("read: %u %p %u\n", fd, buf, count);
	struct open_file *file = get_file_from_fd(fd);
	if (!file || file->type != FILE_REGULAR)
		return (-EBADF);
	int written = read_inode(file->inode.inode_nr, file->inode.file_offset, buf, count);
	if (written > 0)
		file->inode.file_offset += written;
	return (written);
}

uint32_t syscall_write(t_interrupt_data *regs)
{
	int fd = regs->ebx;
	const char *buf = (char *)regs->ecx;
	unsigned int count = regs->edx;
	//printk("write %u %p %u\n", fd, buf, count);
	struct open_file *file = get_file_from_fd(fd);
	if (!file || file->type != FILE_TERMINAL)
		return (-EBADF);

	write(buf, count);
	return (count);
}

uint32_t syscall_chdir(t_interrupt_data *regs) {
	const char *path = (char *)regs->ebx;
	printk("chdir: %s\n", path);
	unsigned int inode_nr = path_to_inode(path, g_curr_task->cwd_inode_nr);
	printk("chdir: inode_nr = %u\n", inode_nr);
	if (!inode_nr) return -ENOENT;

	struct stat stat;
	int status = stat_inode(inode_nr, &stat);
	if (status) {
		return status;
	} else {
		printk("chdir: stat.st_mode = %u\n", stat.st_mode);
		if (stat.st_mode >> 12 != MODE_DIRECTORY) {
			return -ENOTDIR;
		}
	}

	g_curr_task->cwd_inode_nr = inode_nr;
	return 0;
}

struct winsize {
	unsigned short ws_row;
	unsigned short ws_col;
	unsigned short ws_xpixel;  /* unused */
	unsigned short ws_ypixel;  /* unused */
};

#define TCGETS 0x5401
#define TIOCGWINSZ 0x5413

uint32_t syscall_ioctl(t_interrupt_data *regs)
{
	int fd = regs->ebx;
	unsigned long op = regs->ecx;
	printk("ioctl %u %u\n", fd, op);
	struct open_file *file = get_file_from_fd(fd);
	if (!file) return -EBADF;
	if (file->type == FILE_TERMINAL) {
		if (op == TIOCGWINSZ) {
			struct winsize *ws = (struct winsize *)regs->edx;
			ws->ws_row = 25;
			ws->ws_col = 80;
			ws->ws_xpixel = 0;
			ws->ws_ypixel = 0;
			return 0;
		} else if (op == TCGETS) {
			return 0;
		}
	}
	return (-EINVAL);
}

uint32_t syscall_getdents64(t_interrupt_data *regs)
{
	int fd = regs->ebx;
	struct linux_dirent64 *ent = (struct linux_dirent64 *)regs->ecx;
	unsigned int count = regs->edx;
	struct open_file *file = get_file_from_fd(fd);
	printk("getdents64: %d %p %u\n", fd, ent, count);
	if (!file) return -EBADF;

	int written = getdents(file->inode.inode_nr, ent, count, file->inode.file_offset);
	if (written > 0)
		file->inode.file_offset += written;
	return written;
}
