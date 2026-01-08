#include "common.h"
#include "pic.h"
#include "pci.h"
#include "libk/libk.h"

unsigned int rtl_8139_io_base;
unsigned char receive_buffer[8192 + 16 + 1500];

enum io_offset {
	OFF_RBSTART = 0x30,
	OFF_CMD = 0x37,
	OFF_CAPR = 0x38,
	OFF_IMR = 0x3C, // Interrupt Mask Register
	OFF_ISR = 0x3E, // Interrupt Status Register
	OFF_TSD0 = 0x10,
	OFF_TSAD0 = 0x20,
	OFF_RCR = 0x44, // Receive Configuration Register
	OFF_CONFIG1 = 0x52,
};

__attribute__((interrupt)) void rtl8139_handler(__attribute__((unused)) struct interrupt_stack_frame *interrupt_frame)
{
	unsigned int io_base = rtl_8139_io_base;
	unsigned short status = inw(io_base + OFF_ISR);
	unsigned short capr = inw(io_base + OFF_CAPR);
	outw(io_base + OFF_ISR, INT_NIC);
	printk("NIC %u %u %u!\n", capr, io_base, status);
	pic_eoi(INT_NIC);
}

void rtl_8139_init(unsigned char bus, unsigned char slot) {
		unsigned int io_base = (unsigned int)pci_config_read_word(bus, slot, 0, 18) << 16 | pci_config_read_word(bus, slot, 0, 16);
		rtl_8139_io_base = io_base;
		if (!(io_base & 1))
			return;

		// Mark as bus master for DMA
		unsigned short command = pci_config_read_word(bus, slot, 0, 4);
		command |= 0x4;
		pci_config_write_word(bus, slot, 0, 4, command);

		io_base = io_base & IO_BAR_MASK;
		unsigned char our_mac[] = {inb(io_base), inb(io_base + 1), inb(io_base + 2), inb(io_base + 3), inb(io_base + 4), inb(io_base + 5)};
		printk("MAC: %x:%x:%x:%x:%x:%x\n", our_mac[0], our_mac[1], our_mac[2], our_mac[3], our_mac[4], our_mac[5]);
		outb(io_base + OFF_CONFIG1, 0x0); // LWAKE + LWPTN
		outb(io_base + OFF_CMD, 0x10); // Software reset
		while ((inb(io_base + OFF_CMD) & 0x10) != 0) {}

		unsigned char our_ipv4[] = {10, 11, 15, 13};
		unsigned char dst_mac[] = {0xd0, 0x46, 0x0c, 0x85, 0xa6, 0x64};
		unsigned char dst_ipv4[] = {10, 11, 4, 14};

		outl(io_base + OFF_RBSTART, (unsigned int)&receive_buffer[0]); // receive buffer start
		outw(io_base + OFF_IMR, 0x0005); // Transmit OK and Receive OK interrupts
		outl(io_base + OFF_RCR, 0xf | (1<<7)); // Accept all packets, overflow instead of wrap around
		outb(io_base + OFF_CMD, 0x0c); // Enable receive and transmission
		unsigned char frame[] = {
			// Ethernet
			dst_mac[0], dst_mac[1], dst_mac[2], dst_mac[3], dst_mac[4], dst_mac[5], our_mac[0], our_mac[1], our_mac[2], our_mac[3], our_mac[4], our_mac[5], 0x08, 0x00,
			// IPv4
			0x54, 0x00,
			/*len*/0x00, 0x14 + 8,
			0x00, 0x00, 0x00, 0x00,
			64,
			1,
			/* IPv4 checksum */ 0x00, 0x00,
			our_ipv4[0], our_ipv4[1], our_ipv4[2], our_ipv4[3],
			dst_ipv4[0], dst_ipv4[1], dst_ipv4[2], dst_ipv4[3],
			// ICMP
			0x08,
			0x00,
			/* checksum */ 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00,
			// Ethernet FCS
			0x00, 0x00, 0x00, 0x00
		};
		outl(io_base + OFF_TSAD0, (unsigned int)&frame); // transmit start, for now physical = virtual
		outl(io_base + OFF_TSD0, sizeof(frame));
}
