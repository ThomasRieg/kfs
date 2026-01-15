#include "../tty/tty.h"
#include "../common.h"

static unsigned char c;

void handle_serial(void) {
	if (c != 0) {
		tty_add_input(c);
		c = 0;
	}
}

void serial1_handler(__attribute__((unused)) t_interrupt_data *regs)
{
	c = inb(PORT_COM1);
	// pic_eoi(INT_SERIAL1);
}

void serial_writes(const char *str) {
	for (;*str;str++)
		outb(PORT_COM1, *str);
}
