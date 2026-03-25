#include "../syscalls.h"
#include "../../errno.h"
#include "../../libk/libk.h"

#define LINUX_REBOOT_MAGIC1 0xfee1dead
#define LINUX_REBOOT_MAGIC2 0x28121969
#define LINUX_REBOOT_CMD_POWER_OFF 0x4321fedc
#define LINUX_REBOOT_CMD_RESTART 0x1234567

uint32_t syscall_reboot(t_interrupt_data *regs) {
	unsigned int magic = regs->ebx;
	unsigned int magic2 = regs->ecx;
	unsigned int op = regs->edx;
	print_trace("reboot: %u %u %u\n", magic, magic2, op);
	if (magic != LINUX_REBOOT_MAGIC1 || magic2 != LINUX_REBOOT_MAGIC2)
		return -EINVAL;
	if (op == LINUX_REBOOT_CMD_POWER_OFF || op == LINUX_REBOOT_CMD_RESTART) {
		// TODO: proper shutdown
		outw(0x604, 0x2000); // QEMU shutdown only
		return 0;
	}
	return -EINVAL;
}

uint32_t syscall_init_module(t_interrupt_data *regs) {
	const unsigned char *module_image = (const unsigned char *)regs->ebx;
	unsigned int size = regs->ecx;
	const char *param_values = (const char *)regs->edx;

	print_trace("init_module: %p %u %p\n", module_image, size, param_values);
	return -ENOSYS;
}
