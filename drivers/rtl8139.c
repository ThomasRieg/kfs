#include "../common.h"
#include "pic.h"
#include "../net.h"
#include "pci.h"
#include "../tty/tty.h"
#include "../libk/libk.h"
#include "../interrupts/interrupts.h"

struct rtl8139 rtl8139;

enum io_offset
{
	OFF_RBSTART = 0x30, // Read Buffer Start
	OFF_CMD = 0x37,
	OFF_CAPR = 0x38,  // Current Address of Packet Read
	OFF_CBR = 0x3A,  // Current Buffer Address
	OFF_IMR = 0x3C,	  // Interrupt Mask Register
	OFF_ISR = 0x3E,	  // Interrupt Status Register
	OFF_TSD0 = 0x10,  // Transmit Status of Descriptor 0
	OFF_TSAD0 = 0x20, // Transmit Start Address of Descriptor 0
	OFF_TCR = 0x40,	  // Transmit Configuration Register
	OFF_RCR = 0x44,	  // Receive Configuration Register
	OFF_CONFIG1 = 0x52,
};

// From http://realtek.info/pdf/rtl8139d.pdf
enum isr {
	ROK = 1, // In normal mode, indicates the successful completion of a packet recepetion.
			 // In early mode, indicates that the Rx byte count of the arriving packet exceeds the early Rx thresholds.
	RER = 1 << 1, // Indicates that a packet has either CRC error or frame alignment error (FAE). 
				  // The collided frame will not be recognized as CRC error if the length of this frame is shorter than 16 bytes.
	TOK = 1 << 2, // Indicates that a packet transmission is completed successfully.
	TER = 1 << 3, // Indicates that a packet transmission was aborted, due to excessive collisions, according to the TXRR's settings.
	RXOVW = 1 << 4, // Set when receive (Rx) buffer ring storage resources have been exhausted.
	PUN_LINKCHG = 1 << 5, // Set to 1 when CAPR is written but Rx buffer is empty, or when link status is changed.
	FOVW = 1 << 6, // Set when an overflow occurs on the Rx status FIFO
	LENCHG = 1 << 13, // Cable length is changed after Receiver is enabled.
	TIMEOUT = 1 << 14, // Set to 1 when the TCTR register reaches to the value of the TimerInt register.
	SERR = 1 << 15, // Set to 1 when the RTL8139D(L) signals a system error on the PCI bus.
};

void rtl8139_handler(__attribute__((unused)) t_interrupt_data *regs)
{
	unsigned int io_base = rtl8139.io_base;
	unsigned short status = inw(io_base + OFF_ISR);
	// official documentation says writing to ISR
	// has no effect but it seems to be mandatory
	// to mark the interrupt as handled
	outw(io_base + OFF_ISR, 0xFFFF);
	unsigned short capr = inw(io_base + OFF_CAPR);
	unsigned short cbr = inw(io_base + OFF_CBR);
	printk("NIC io_base: 0x%x, status 0x%x (", io_base, status);
	if (status & ROK)
		writes("ROK ");
	if (status & RER) {
		writes("RER ");
	}
	if (status & TOK) {
		writes("TOK ");
	}
	if (status & TER) {
		writes("TER ");
	}
	if (status & RXOVW) {
		writes("RXOVW ");
	}
	if (status & PUN_LINKCHG) {
		writes("PUN_LINKCHG ");
	}
	printk("), CBR 0x%x, CAPR: 0x%x\n", cbr, capr);
	if (status & ROK)
	{
		unsigned char *start = rtl8139.receive_buffer + (unsigned short)(capr + 16);
		unsigned short length = *(unsigned short *)(start + 2);
		printk("length %d\n", length);
		struct ether *ether = (struct ether *)(start + 4);
		handle_frame(ether);
		// print whole packet with status
		hex_dump(start, length + 4);
		outw(io_base + OFF_CAPR, start - rtl8139.receive_buffer + 4 + length - 16); // mark as read up until now
	}
	pic_eoi(INT_NIC);
}

void rtl_8139_transmit(void *frame, unsigned int size) {
	unsigned int buffer_i = rtl8139.next_send_buffer;
	void *send_buffer = rtl8139.send_buffers[rtl8139.next_send_buffer];
	rtl8139.next_send_buffer = (rtl8139.next_send_buffer + 1) % 4;

	unsigned int io_base = rtl8139.io_base;
	memcpy(send_buffer, frame, size);
	outl(io_base + OFF_TSAD0 + buffer_i * 4, (unsigned int)send_buffer); // transmit start, for now physical = virtual
	outl(io_base + OFF_TSD0 + buffer_i * 4, size);
}

void rtl_8139_init(struct pci_installed *installed)
{
	unsigned int io_base = (unsigned int)pci_config_read_word(installed->bus, installed->slot, 0, 18) << 16 | pci_config_read_word(installed->bus, installed->slot, 0, 16);
	if (!(io_base & 1))
		return;

	// Mark as bus master for DMA
	unsigned short command = pci_config_read_word(installed->bus, installed->slot, 0, 4);
	command |= 0x4;
	pci_config_write_word(installed->bus, installed->slot, 0, 4, command);

	io_base = io_base & IO_BAR_MASK;
	rtl8139.io_base = io_base;
	memcpy(rtl8139.ipv4, "\xC0\xA8\x4C\x09", 4); // 192.168.76.9
	memcpy(rtl8139.gateway_ipv4, "\xC0\xA8\x4C\x02", 4); // 192.168.76.2
	unsigned char our_mac[] = {inb(io_base), inb(io_base + 1), inb(io_base + 2), inb(io_base + 3), inb(io_base + 4), inb(io_base + 5)};
	memcpy(rtl8139.mac, our_mac, 6);
	printk("MAC: %x:%x:%x:%x:%x:%x\n", our_mac[0], our_mac[1], our_mac[2], our_mac[3], our_mac[4], our_mac[5]);
	printk("IPv4: %u.%u.%u.%u\n", rtl8139.ipv4[0], rtl8139.ipv4[1], rtl8139.ipv4[2], rtl8139.ipv4[3]);
	printk("Gateway IPv4: %u.%u.%u.%u\n", rtl8139.gateway_ipv4[0], rtl8139.gateway_ipv4[1], rtl8139.gateway_ipv4[2], rtl8139.gateway_ipv4[3]);
	outb(io_base + OFF_CONFIG1, 0x0); // LWAKE + LWPTN
	outb(io_base + OFF_CMD, 0x10);	  // Software reset
	while ((inb(io_base + OFF_CMD) & 0x10) != 0)
	{
		// wait for software reset to happen.
	}

	outl(io_base + OFF_RBSTART, (unsigned int)&rtl8139.receive_buffer[0]); // receive buffer start
	outw(io_base + OFF_IMR, 0xffff);							   // receive all interrupts
	outl(io_base + OFF_TCR, 1 << 15);							   // Append Ethernet CRC
	outl(io_base + OFF_RCR, 0xf | (1 << 7));					   // Accept all packets, overflow instead of wrap around
	outb(io_base + OFF_CMD, 0x0c);								   // Enable receive and transmission
}
