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

struct stat {
    unsigned int st_dev;
    unsigned int st_ino;
    unsigned int st_nlink;
    unsigned int st_mode;
    unsigned int st_uid;
    unsigned int st_gid;
    int __pad0;
    unsigned int st_rdev;
    unsigned int st_size;
    unsigned int st_blksize;
    unsigned int st_blocks;
    struct timespec st_atim;
    struct timespec st_mtim;
    struct timespec st_ctim;
};

static struct open_file *get_file_from_fd(int fd) {
	struct open_file *file = fd >= 0 && (unsigned int)fd < MAX_OPEN_FILES ? g_curr_task->open_files[fd] : 0;
	return file;
}

uint32_t syscall_statx(t_interrupt_data *regs) {
	unsigned int flags = regs->edx;
	void *buffer = (void*)regs->edi;
	printk("statx at eip %p: %u %s %u %u %p\n", regs->eip, regs->ebx, regs->ecx, flags, regs->esi, buffer);
	if (*(unsigned char *)regs->ecx == 0 && !(flags & AT_EMPTY_PATH))
		return (-ENOENT);
	memset(buffer, 0, 256);
	return (0);
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
	printk("inode_nr: %u\n", inode_nr);
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
	struct open_file *file = fd >= 0 && (unsigned int)fd < MAX_OPEN_FILES ? g_curr_task->open_files[fd] : 0;
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
	struct open_file *file = fd >= 0 && (unsigned int)fd < MAX_OPEN_FILES ? g_curr_task->open_files[fd] : 0;
	if (!file || file->type != FILE_TERMINAL)
		return (-EBADF);

	write(buf, count);
	return (count);
}
