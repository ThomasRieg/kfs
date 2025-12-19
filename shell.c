#include "libk/libk.h"
#include "vga/vga.h"
#include "io.h"

void handle_command(unsigned char len, const char *cmd) {
	if (len == 4 && memcmp(cmd, "stop", 4) == 0) {
		writes("stopping...\n");
		// TODO: proper shutdown
		outw(0x604, 0x2000); // QEMU shutdown only
	}
	else if (len)
		writes("command not found!\n");

	writes("> ");
}

