#include "libk/libk.h"
#include "tty/tty.h"
#include "io.h"
#include "pci.h"
#include "common.h"

void print_clock(void);

void handle_command(unsigned char len, const char *cmd)
{
	bool found = true;

	switch (len)
	{
	case 3:
		if (memcmp(cmd, "pci", 3) == 0)
		{
			pci_enumerate();
		}
		else
			found = false;
		break;
	case 4:
		if (memcmp(cmd, "stop", 4) == 0)
		{
			writes("stopping...\n");
			// TODO: proper shutdown
			outw(0x604, 0x2000); // QEMU shutdown only
		}
		else if (memcmp(cmd, "help", 4) == 0)
		{
			writes("help:\n"
				   "- stop: power off the machine\n"
				   "- date: print RTC/wall-clock date and time\n"
				   "- print stack: print the whole stack bye per byte in hexadecimal\n"
				   "- print trace: print call backtrace\n"
				   "- crash: crash the kernel by accessing unmapped memory\n"
				   "- clear: clear TTY\n"
				   "- breakpoint: cause the breakpoint instruction to be executed\n"
				   "- pci: enumerate PCI devices on bus 0 with function 0\n");
		}
		else if (memcmp(cmd, "date", 4) == 0)
		{
			print_clock();
		}
		else
			found = false;
		break;
	case 5:
		if (memcmp(cmd, "clear", 5) == 0)
			clear_vga_screen();
		else if (memcmp(cmd, "crash", 5) == 0)
		{
			*((unsigned char *)4242424242) = 5;
		}
		else
			found = false;
		break;
	case 10:
		if (memcmp(cmd, "breakpoint", 10) == 0)
		{
			asm volatile("int3"); // breakpoint
		}
		else
			found = false;
		break;
	case 11:
		if (memcmp(cmd, "print stack", 11) == 0)
		{
			char forty_two[100];
			memset(forty_two, 0x42, 100);
			stack_dump_words(0);
		}
		else if (memcmp(cmd, "print trace", 11) == 0)
		{
			stack_trace_ebp(32);
		}
		else
			found = false;
		break;
	default:
		found = false;
	}
	if (!found && len)
		writes("command not found!\n");

	writes("> ");
}
