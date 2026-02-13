#include "../syscalls.h"
#include "../../tasks/task.h"
#include "../../vmalloc/vmalloc.h"
#include "../../libk/libk.h"
#include "../../mmap/mmap.h"
#include "../../mem_page/mem_paging.h"
#include "../../errno.h"
#include "../../ext2.h"
#include "../../tty/tty.h"
#include "../../fd/fd.h"
#include "../../fd/inode.h"
#include "../../fd/fd_tty.h"
#include "../../fd/pipe.h"

#define AT_SYMLINK_NOFOLLOW 0x100
#define AT_EMPTY_PATH 0x1000
#define AT_FDCWD -100

struct statx_timestamp
{
	int64_t tv_sec;		  /* Seconds since the Epoch (UNIX time) */
	unsigned int tv_nsec; /* Nanoseconds since tv_sec */
	int _reserved;
};

struct statx
{
	unsigned int stx_mask;	  /* Mask of bits indicating
filled fields */
	unsigned int stx_blksize; /* Block size for filesystem I/O */
	uint64_t stx_attributes;  /* Extra file attribute indicators */
	unsigned int stx_nlink;	  /* Number of hard links */
	unsigned int stx_uid;	  /* User ID of owner */
	unsigned int stx_gid;	  /* Group ID of owner */
	unsigned short stx_mode;  /* File type and mode */
	uint64_t stx_ino;		  /* Inode number */
	uint64_t stx_size;		  /* Total size in bytes */
	uint64_t stx_blocks;	  /* Number of 512B blocks allocated */
	uint64_t stx_attributes_mask;
	/* Mask to show what's supported
	in stx_attributes */

	/* The following fields are file timestamps */
	struct statx_timestamp stx_atime; /* Last access */
	struct statx_timestamp stx_btime; /* Creation */
	struct statx_timestamp stx_ctime; /* Last status change */
	struct statx_timestamp stx_mtime; /* Last modification */

	/* If this file represents a device, then the next two
	fields contain the ID of the device */
	unsigned int stx_rdev_major; /* Major ID */
	unsigned int stx_rdev_minor; /* Minor ID */

	/* The next two fields contain the ID of the device
	containing the filesystem where the file resides */
	unsigned int stx_dev_major; /* Major ID */
	unsigned int stx_dev_minor; /* Minor ID */

	uint64_t stx_mnt_id; /* Mount ID */

	/* Direct I/O alignment restrictions */
	unsigned int stx_dio_mem_align;
	unsigned int stx_dio_offset_align;

	uint64_t stx_subvol; /* Subvolume identifier */

	/* Direct I/O atomic write limits */
	unsigned int stx_atomic_write_unit_min;
	unsigned int stx_atomic_write_unit_max;
	unsigned int stx_atomic_write_segments_max;

	/* File offset alignment for direct I/O reads */
	unsigned int stx_dio_read_offset_align;

	/* Direct I/O atomic write max opt limit */
	unsigned int stx_atomic_write_unit_max_opt;
};

_Static_assert(sizeof(struct statx) <= 256, "struct statx is overflowing glibc definition");

static t_file *get_file_from_fd(int fd)
{
	t_file *file = fd >= 0 && (unsigned int)fd < MAX_OPEN_FILES ? g_curr_task->open_files[fd] : 0;
	return file;
}

#define STATX_TYPE 0x00000001U		  /* Want/got stx_mode & S_IFMT */
#define STATX_MODE 0x00000002U		  /* Want/got stx_mode & ~S_IFMT */
#define STATX_NLINK 0x00000004U		  /* Want/got stx_nlink */
#define STATX_UID 0x00000008U		  /* Want/got stx_uid */
#define STATX_GID 0x00000010U		  /* Want/got stx_gid */
#define STATX_ATIME 0x00000020U		  /* Want/got stx_atime */
#define STATX_MTIME 0x00000040U		  /* Want/got stx_mtime */
#define STATX_CTIME 0x00000080U		  /* Want/got stx_ctime */
#define STATX_INO 0x00000100U		  /* Want/got stx_ino */
#define STATX_SIZE 0x00000200U		  /* Want/got stx_size */
#define STATX_BLOCKS 0x00000400U	  /* Want/got stx_blocks */
#define STATX_BASIC_STATS 0x000007ffU /* The stuff in the normal stat struct */
#define STATX_BTIME 0x00000800U		  /* Want/got stx_btime */
#define STATX_MNT_ID 0x00001000U	  /* Got stx_mnt_id */

static void fill_statx_from_stat(struct statx *statx, struct stat *stat)
{
	statx->stx_ino = stat->st_ino;
	statx->stx_nlink = stat->st_nlink;
	statx->stx_uid = stat->st_uid;
	statx->stx_gid = stat->st_gid;
	statx->stx_blocks = stat->st_blocks;
	statx->stx_size = stat->st_size;
	statx->stx_mode = stat->st_mode;
	statx->stx_atime = (struct statx_timestamp){.tv_sec=stat->st_atim.tv_sec};
	statx->stx_ctime = (struct statx_timestamp){.tv_sec=stat->st_ctim.tv_sec};
	statx->stx_mtime = (struct statx_timestamp){.tv_sec=stat->st_mtim.tv_sec};
}

uint32_t syscall_statx(t_interrupt_data *regs)
{
	int dirfd = regs->ebx;
	const char *path = (char *)regs->ecx;
	unsigned int flags = regs->edx;
	unsigned int mask = regs->esi;
	struct statx *buffer = (struct statx *)regs->edi;
	if (!user_str_ok(path, false, 20000, &g_curr_task->proc_memory))
		return (-EFAULT);
	if (!user_range_ok(buffer, sizeof(*buffer), true, &g_curr_task->proc_memory))
		return (-EFAULT);
	print_trace("statx at eip %p: %u %s %u %u %p\n", regs->eip, dirfd, path, flags, mask, buffer);
	if (path[0] == 0)
	{
		if (!(flags & AT_EMPTY_PATH))
			return (-ENOENT);
		t_file *file = get_file_from_fd(dirfd);
		if (!file)
			return -EBADF;

		*buffer = (struct statx){
			.stx_mask = STATX_BASIC_STATS | STATX_MNT_ID,
			.stx_mode = (file->type == FILE_TERMINAL ? MODE_CHAR : MODE_REGULAR) << 12,
			.stx_ino = (file->type == FILE_REGULAR ? ((t_inode *)file->priv)->inode_nr : 0),
			.stx_mnt_id = 0,
		};
		if (file->type == FILE_REGULAR)
		{
			struct stat stat;
			int status = stat_inode(((t_inode *)file->priv)->inode_nr, &stat);
			if (status)
				return status;
			fill_statx_from_stat(buffer, &stat);
		}
		return 0;
	}
	int inode_nr = path_to_inode(path, g_curr_task->cwd_inode_nr);
	if (!inode_nr)
		return -ENOENT;
	struct stat stat;
	int status = stat_inode(inode_nr, &stat);
	if (status)
		return status;
	*buffer = (struct statx){
		.stx_mask = STATX_BASIC_STATS | STATX_MNT_ID,
		.stx_mnt_id = 0,
	};
	fill_statx_from_stat(buffer, &stat);
	return 0;
}

uint32_t syscall_fstatat(t_interrupt_data *regs)
{
	int dirfd = regs->ebx;
	const char *path = (char *)regs->ecx;
	struct stat *buf = (struct stat *)regs->edx;
	unsigned int flags = regs->esi;
	print_trace("fstatat at eip %p: %u %s %p %u\n", regs->eip, dirfd, path, buf, flags);
	if (!user_range_ok(buf, sizeof(*buf), true, &g_curr_task->proc_memory))
		return (-EFAULT);
	if (!user_str_ok(path, false, 20000, &g_curr_task->proc_memory))
		return (-EFAULT);
	if (path[0] == 0 && !(flags & AT_EMPTY_PATH))
		return (-ENOENT);
	memset(buf, 0, sizeof(struct stat));
	return (0);
}

uint32_t syscall_fstat64(t_interrupt_data *regs)
{
	int fd = regs->ebx;
	struct stat64 *buf = (struct stat64 *)regs->ecx;
	print_trace("fstat64: %u %p\n", fd, buf);
	if (!user_range_ok(buf, sizeof(*buf), true, &g_curr_task->proc_memory))
		return (-EFAULT);
	t_file *file = get_file_from_fd(fd);
	if (!file || file->type != FILE_REGULAR)
		return -EBADF;
	struct stat stat;
	int status = stat_inode(((t_inode*)file->priv)->inode_nr, &stat);
	if (status)
		return status;
	*buf = (struct stat64){
		.st_ino = stat.st_ino,
		.st_mode = stat.st_mode,
		.st_size = stat.st_size,
		.st_blocks = stat.st_blocks,
		.st_uid = stat.st_uid,
		.st_gid = stat.st_gid,
		.st_nlink = stat.st_nlink,
		.st_atim = stat.st_atim,
		.st_ctim = stat.st_ctim,
		.st_mtim = stat.st_mtim};
	return (0);
}

uint32_t syscall_readlink(t_interrupt_data *regs)
{
	const char *path = (char *)regs->ebx;
	char *buf = (char *)regs->ecx;
	unsigned int buf_size = regs->edx;
	if (!user_range_ok(buf, buf_size, true, &g_curr_task->proc_memory))
		return (-EFAULT);
	print_trace("readlink: %s %p %u\n", path, buf, buf_size);
	return (-ENOSYS);
}

uint32_t do_open(const char *path, unsigned int dir_inode, int flags, __attribute__((unused)) unsigned int mode)
{
	unsigned short i;
	for (i = 0; i < MAX_OPEN_FILES; i++)
	{
		if (!g_curr_task->open_files[i])
			break;
	}
	if (i >= MAX_OPEN_FILES)
		return (-EMFILE);
	print_debug("open: free fd=%u\n", i);
	if (strcmp(path, "/dev/tty1") == 0)
	{
		t_file *file = vmalloc(sizeof(*file));
		if (!file)
			return (-ENOMEM);
		file->refcnt = 1;
		file->type = FILE_TERMINAL;
		file->ops = &g_tty_ops;
		file->pos = 0;
		file->flags = flags;
		file->priv = &g_ttys[0]; // tty1 = tty 0 I guess?
		g_curr_task->open_files[i] = file;
		return i;
	}
	unsigned int inode_nr = path_to_inode(path, dir_inode);
	print_debug("open inode_nr: %u\n", inode_nr);
	if (!inode_nr)
		return -ENOENT;

	t_file *file = vmalloc(sizeof(*file));
	if (!file)
		return (-ENOMEM);
	file->refcnt = 1;
	file->type = FILE_REGULAR;
	file->flags = flags;
	t_inode *inode = vmalloc(sizeof(t_inode));
	if (!inode)
		return (vfree(file), -ENOMEM);
	inode->file_offset = 0;
	inode->inode_nr = inode_nr;
	file->priv = inode;
	file->ops = &g_inode_ops;
	file->pos = 0;

	g_curr_task->open_files[i] = file;
	return i;
}

uint32_t syscall_open(t_interrupt_data *regs)
{
	const char *path = (char *)regs->ebx;
	int flags = regs->ecx;
	unsigned int mode = regs->edx;
	print_trace("open: %s %d %u\n", path, flags, mode);
	if (!path)
		return -EFAULT;
	if (!user_str_ok(path, false, 20000, &g_curr_task->proc_memory))
		return (-EFAULT);
	return do_open(path, g_curr_task->cwd_inode_nr, flags, mode);
}

uint32_t syscall_openat(t_interrupt_data *regs)
{
	int dirfd = regs->ebx;
	const char *path = (char *)regs->ecx;
	int open_flag = regs->edx;
	unsigned int mode = regs->edi;
	print_trace("openat: %u %s %u %u\n", dirfd, path, open_flag, mode);
	if (!path)
		return -EFAULT;
	if (!user_str_ok(path, false, 20000, &g_curr_task->proc_memory))
		return (-EFAULT);
	unsigned int dir_inode;
	if (dirfd == AT_FDCWD)
		dir_inode = g_curr_task->cwd_inode_nr;
	else
	{
		t_file *file = get_file_from_fd(dirfd);
		if (!file || file->type != FILE_REGULAR)
			return -EBADF;
		dir_inode = ((t_inode *)file->priv)->inode_nr;
	}
	return do_open(path, dir_inode, open_flag, mode);
}

uint32_t syscall_dup2(t_interrupt_data *regs)
{
	int oldfd = regs->ebx;
	int newfd = regs->ecx;
	print_trace("dup2: %u %u\n", oldfd, newfd);
	t_file *file = get_file_from_fd(oldfd);
	if (!file)
		return (-EBADF);
	if (newfd < 0 || (unsigned)newfd >= MAX_OPEN_FILES)
		return -EBADF;
	t_file *newfile = g_curr_task->open_files[newfd];
	if (newfile) {
		newfile->refcnt--;
		if (newfile->refcnt == 0)
		{
			newfile->ops->close(newfile);
			vfree(newfile);
		}
	}
	g_curr_task->open_files[newfd] = file;
	file->refcnt += 1;
	return (newfd);
}

// TODO adapt to new file struct
uint32_t syscall_dup(t_interrupt_data *regs)
{
	int fd = regs->ebx;
	print_trace("dup: %u\n", fd);
	t_file *file = get_file_from_fd(fd);
	if (!file)
		return (-EBADF);
	unsigned short i;
	for (i = 0; i < MAX_OPEN_FILES; i++)
	{
		if (!g_curr_task->open_files[i])
			break;
	}
	if (i >= MAX_OPEN_FILES)
		return (-EMFILE);
	g_curr_task->open_files[i] = file;
	file->refcnt += 1;
	return (i);
}

uint32_t syscall_close(t_interrupt_data *regs)
{
	int fd = regs->ebx;
	print_trace("close: %u\n", fd);
	t_file *file = get_file_from_fd(fd);
	if (!file)
		return (-EBADF);
	if (!file->ops->close)
		return (-EINVAL);
	file->refcnt--;
	if (file->refcnt == 0)
	{
		file->ops->close(file); // doesn't free file
		vfree(file);
	}
	g_curr_task->open_files[fd] = NULL;
	return (0);
}

#define F_DUPFD 0
#define F_DUPFD_CLOEXEC 1030
uint32_t syscall_fcntl64(t_interrupt_data *regs)
{
	int fd = regs->ebx;
	int cmd = regs->ecx;
	print_trace("fcntl64: %u %u\n", fd, cmd);
	t_file *file = get_file_from_fd(fd);
	if (!file)
		return (-EBADF);
	if (cmd == F_DUPFD || cmd == F_DUPFD_CLOEXEC) {
		// TODO: handle CLOEXEC
		int ge_fd = regs->edx;
		print_debug("fcntl64 ge_fd=%u\n", ge_fd);
		if (ge_fd < 0 || (unsigned)ge_fd >= MAX_OPEN_FILES)
			return -EBADF;
		while ((unsigned int)ge_fd < MAX_OPEN_FILES && g_curr_task->open_files[ge_fd])
			ge_fd++;
		if ((unsigned int)ge_fd >= MAX_OPEN_FILES)
			return -EMFILE;
		print_debug("fcntl free_ge_fd=%u\n", ge_fd);
		g_curr_task->open_files[ge_fd] = file;
		file->refcnt += 1;
		return ge_fd;
	}
	return -EINVAL;
}

uint32_t syscall_read(t_interrupt_data *regs)
{
	unsigned int fd = regs->ebx;
	char *buf = (char *)regs->ecx;
	unsigned int count = regs->edx;
	if (!user_range_ok(buf, count, true, &g_curr_task->proc_memory))
		return (-EFAULT);
	print_trace("read: %u %p %u\n", fd, buf, count);
	t_file *file = get_file_from_fd(fd);
	if (!file)
		return (-EBADF);
	if (!file->ops->read)
		return (-EINVAL);
	return (file->ops->read(file, buf, count));
}

uint32_t syscall_write(t_interrupt_data *regs)
{
	int32_t fd = regs->ebx;
	const char *buf = (char *)regs->ecx;
	uint32_t count = regs->edx;
	if (!user_range_ok((const virt_ptr)buf, count, false, &g_curr_task->proc_memory))
		return (-EFAULT);
	print_trace("write %u %p %u\n", fd, buf, count);
	t_file *file = get_file_from_fd(fd);
	if (!file)
		return (-EBADF);
	if (!file->ops->write)
		return (-EINVAL);
	return (file->ops->write(file, buf, count));
}

uint32_t do_pipe(int32_t *pipe_fd, uint32_t flags) {
	if (!user_range_ok(pipe_fd, 2 * sizeof(int32_t), true, &g_curr_task->proc_memory))
		return (-EFAULT);
	uint32_t i;
	uint32_t j;
	for (i = 0; i < MAX_OPEN_FILES; i++)
	{
		if (!g_curr_task->open_files[i])
			break;
	}
	for (j = i + 1; j < MAX_OPEN_FILES; j++)
	{
		if (!g_curr_task->open_files[j])
			break;
	}
	if (j >= MAX_OPEN_FILES)
		return (-EMFILE);
	t_pipe *pipe = vcalloc(1, sizeof(*pipe));
	if (!pipe)
		return (-ENOMEM);
	t_pipe_end *pipe_end_read = vmalloc(sizeof(*pipe_end_read));
	if (!pipe_end_read)
		return (vfree(pipe), -ENOMEM);
	t_pipe_end *pipe_end_write = vmalloc(sizeof(*pipe_end_write));
	if (!pipe_end_read)
		return (vfree(pipe), vfree(pipe_end_read), -ENOMEM);
	pipe_end_write->is_read_end = 0;
	pipe_end_write->p = pipe;
	pipe->readers = 1;
	pipe->writers = 1;
	pipe_end_read->is_read_end = 1;
	pipe_end_read->p = pipe;
	t_file *file_read = vcalloc(1, sizeof(*file_read));
	if (!file_read)
		return (vfree(pipe), vfree(pipe_end_read), vfree(pipe_end_write), -ENOMEM);
	t_file *file_write = vcalloc(1, sizeof(*file_write));
	if (!file_write)
		return (vfree(pipe), vfree(pipe_end_read), vfree(pipe_end_write), vfree(file_read), -ENOMEM);
	file_read->flags = O_RDONLY | flags;
	file_read->ops = &g_pipe_read_ops;
	file_read->pos = 0;
	file_read->priv = pipe_end_read;
	file_read->refcnt = 1;
	file_read->type = FILE_PIPE;
	file_write->flags = O_WRONLY | flags;
	file_write->ops = &g_pipe_write_ops;
	file_write->pos = 0;
	file_write->priv = pipe_end_write;
	file_write->refcnt = 1;
	file_write->type = FILE_PIPE;
	g_curr_task->open_files[i] = file_read;
	pipe_fd[0] = i;
	g_curr_task->open_files[j] = file_write;
	pipe_fd[1] = j;
	return (0);
}

uint32_t syscall_pipe(t_interrupt_data *regs)
{
	int32_t *pipe_fd = (int32_t *)regs->ebx; //validity of pointer verified in do_pipe
	return do_pipe(pipe_fd, 0);
}

uint32_t syscall_pipe2(t_interrupt_data *regs)
{
	int32_t *pipe_fd = (int32_t *)regs->ebx; //validity of pointer verified in do_pipe
	int flags = (int)regs->ecx;
	return do_pipe(pipe_fd, flags);
}

uint32_t syscall_chdir(t_interrupt_data *regs)
{
	const char *path = (char *)regs->ebx;
	if (!user_str_ok(path, false, 20000, &g_curr_task->proc_memory))
		return (-EFAULT);
	print_trace("chdir: %s\n", path);
	unsigned int inode_nr = path_to_inode(path, g_curr_task->cwd_inode_nr);
	print_debug("chdir: inode_nr = %u\n", inode_nr);
	if (!inode_nr)
		return -ENOENT;

	struct stat stat;
	int status = stat_inode(inode_nr, &stat);
	if (status)
	{
		return status;
	}
	else
	{
		print_debug("chdir: stat.st_mode = %u\n", stat.st_mode);
		if (stat.st_mode >> 12 != MODE_DIRECTORY)
		{
			return -ENOTDIR;
		}
	}

	g_curr_task->cwd_inode_nr = inode_nr;
	return 0;
}

// TODO add this to file->ops instead of doing a switch in this function
uint32_t syscall_ioctl(t_interrupt_data *regs)
{
	int fd = regs->ebx;
	unsigned long op = regs->ecx;
	print_trace("ioctl %u: 0x%x = %u\n", fd, op, op);
	t_file *file = get_file_from_fd(fd);
	if (!file)
		return -EBADF;
	if (file->ops->ioctl)
		return file->ops->ioctl(file, op, regs->edx);
	return (-EINVAL);
}

uint32_t syscall_getdents64(t_interrupt_data *regs)
{
	int fd = regs->ebx;
	struct linux_dirent64 *ent = (struct linux_dirent64 *)regs->ecx;
	unsigned int count = regs->edx;
	if (!user_range_ok(ent, count, true, &g_curr_task->proc_memory))
		return (-EFAULT);
	t_file *file = get_file_from_fd(fd);
	print_trace("getdents64: %d %p %u\n", fd, ent, count);
	if (!file)
		return -EBADF;

	int written = getdents(((t_inode *)file->priv)->inode_nr, ent, count, ((t_inode *)file->priv)->file_offset);
	if (written > 0)
		((t_inode *)file->priv)->file_offset += written;
	return written;
}

uint32_t syscall_getcwd(t_interrupt_data *regs) {
	char *buf = (char *)regs->ebx;
	unsigned int size = regs->ecx;
	print_trace("getcwd: %p %u\n", buf, size);
	if (!user_range_ok(buf, size, true, &g_curr_task->proc_memory))
		return (-EFAULT);
	return getdirname(g_curr_task->cwd_inode_nr, buf, size);
}
