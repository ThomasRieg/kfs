#include "../syscalls.h"
#include "../../tasks/task.h"
#include "../../vmalloc/vmalloc.h"
#include "../../libk/libk.h"
#include "../../mmap/mmap.h"
#include "../../mem_page/mem_paging.h"
#include "../../errno.h"
#include "../../tty/tty.h"

#define AT_EMPTY_PATH 0x1000

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
	unsigned int flags = regs->esi;
	printk("fstatat at eip %p: %u %s %p %u\n", regs->eip, regs->ebx, regs->ecx, regs->edx, flags);
	if (*(unsigned char *)regs->ecx == 0 && !(flags & AT_EMPTY_PATH))
		return (-ENOENT);
	memset((void *)regs->edx, 0, 88);
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
	int fd = regs->ebx;
	const char *path = (char *)regs->ecx;
	int open_flag = regs->edx;
	printk("openat: %u %s %u\n", fd, path, open_flag);
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
	return (-ENOENT);
}

uint32_t syscall_dup(t_interrupt_data *regs) {
	int fd = regs->ebx;
	printk("dup: %u\n", fd);
	struct open_file *file = fd >= 0 && (unsigned int)fd < MAX_OPEN_FILES ? g_curr_task->open_files[fd] : 0;
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
	g_curr_task->open_files[i]->ref_count += 1;
	return (i);
}

