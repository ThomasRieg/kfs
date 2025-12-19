#include "io.h"

// 8259 PIC
// Diagrams stolen from https://os.phil-opp.com/hardware-interrupts/
//                                    ____________             _____
//               Timer ------------> |            |           |     |
//               Keyboard ---------> | Interrupt  |---------> | CPU |
//               Other Hardware ---> | Controller |           |_____|
//               Etc. -------------> |____________|
//
//                      ____________                          ____________
// Real Time Clock --> |            |   Timer -------------> |            |
// ACPI -------------> |            |   Keyboard-----------> |            |      _____
// Available --------> | Secondary  |----------------------> | Primary    |     |     |
// Available --------> | Interrupt  |   Serial Port 2 -----> | Interrupt  |---> | CPU |
// Mouse ------------> | Controller |   Serial Port 1 -----> | Controller |     |_____|
// Co-Processor -----> |            |   Parallel Port 2/3 -> |            |
// Primary ATA ------> |            |   Floppy disk -------> |            |
// Secondary ATA ----> |____________|   Parallel Port 1----> |____________|

enum pic_command {
	PIC_CMD_INIT = 0x11,
	PIC_CMD_END_OF_INTERRUPT = 0x20
};

void pic_eoi(unsigned char irq) {
	if (irq >= PIC_OFFSET + 8)
		outb(PORT_PIC_SECONDARY_CMD, PIC_CMD_END_OF_INTERRUPT);
	outb(PORT_PIC_PRIMARY_CMD, PIC_CMD_END_OF_INTERRUPT);
}

// ICW = Initialization Command Word
#define ICW1_ICW4	0x01		/* Indicates that ICW4 will be present */
#define ICW1_SINGLE	0x02		/* Single (cascade) mode */
#define ICW1_INTERVAL4	0x04		/* Call address interval 4 (8) */
#define ICW1_LEVEL	0x08		/* Level triggered (edge) mode */
#define ICW1_INIT	0x10		/* Initialization - required! */

#define ICW4_8086	0x01		/* 8086/88 (MCS-80/85) mode */
#define ICW4_AUTO	0x02		/* Auto (normal) EOI */
#define ICW4_BUF_SLAVE	0x08		/* Buffered mode/slave */
#define ICW4_BUF_MASTER	0x0C		/* Buffered mode/master */
#define ICW4_SFNM	0x10		/* Special fully nested (not) */

#define CASCADE_IRQ 2

void setup_pics(void) {
	int offset1 = PIC_OFFSET;
	int offset2 = offset1 + 8;
	outb(PORT_PIC_PRIMARY_CMD, ICW1_INIT | ICW1_ICW4);  // starts the initialization sequence (in cascade mode)
	io_wait();
	outb(PORT_PIC_SECONDARY_CMD, ICW1_INIT | ICW1_ICW4);
	io_wait();
	outb(PORT_PIC_PRIMARY_DATA, offset1);                 // ICW2: Master PIC vector offset
	io_wait();
	outb(PORT_PIC_SECONDARY_DATA, offset2);                 // ICW2: Slave PIC vector offset
	io_wait();
	outb(PORT_PIC_PRIMARY_DATA, 1 << CASCADE_IRQ);        // ICW3: tell Master PIC that there is a slave PIC at IRQ2
	io_wait();
	outb(PORT_PIC_SECONDARY_DATA, 2);                       // ICW3: tell Slave PIC its cascade identity (0000 0010)
	io_wait();

	outb(PORT_PIC_PRIMARY_DATA, ICW4_8086);               // ICW4: have the PICs use 8086 mode (and not 8080 mode)
	io_wait();
	outb(PORT_PIC_SECONDARY_DATA, ICW4_8086);
	io_wait();

	// Unmask both PICs.
	outb(PORT_PIC_PRIMARY_DATA, 0);
	outb(PORT_PIC_SECONDARY_DATA, 0);
}
