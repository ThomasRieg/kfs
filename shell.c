#include "libk/libk.h"
#include "vga/vga.h"
#include "io.h"
#include "common.h"

void print_clock(void);

void handle_command(unsigned char len, const char *cmd) {
	bool found = true;

	switch (len) {
	case 4:
		if (memcmp(cmd, "stop", 4) == 0) {
			writes("stopping...\n");
			// TODO: proper shutdown
			outw(0x604, 0x2000); // QEMU shutdown only
		} else if (memcmp(cmd, "help", 4) == 0) {
			writes("help:\n"
				"- stop: power off the machine\n"
				"- date: print RTC/wall-clock date and time\n");
		} else if (memcmp(cmd, "date", 4) == 0) {
			print_clock();
		} else
			found = false;
		break;
	default:
		found = false;
	}
	if (!found && len)
		writes("command not found!\n");

	writes("> ");
}

