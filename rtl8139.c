#include "common.h"
#include "pic.h"
#include "pci.h"
#include "libk/libk.h"

unsigned int rtl_8139_io_base;
unsigned char receive_buffer[8192 + 16 + 1500];
unsigned char send_buffer[1700];

enum io_offset {
	OFF_RBSTART = 0x30, // Read Buffer Start
	OFF_CMD = 0x37,
	OFF_CAPR = 0x38, // Current Address of Packet Read
	OFF_IMR = 0x3C, // Interrupt Mask Register
	OFF_ISR = 0x3E, // Interrupt Status Register
	OFF_TSD0 = 0x10, // Transmit Status of Descriptor 0
	OFF_TSAD0 = 0x20, // Transmit Start Address of Descriptor 0
	OFF_TCR = 0x40, // Transmit Configuration Register
	OFF_RCR = 0x44, // Receive Configuration Register
	OFF_CONFIG1 = 0x52,
};

__attribute__((interrupt)) void rtl8139_handler(__attribute__((unused)) struct interrupt_stack_frame *interrupt_frame)
{
	unsigned int io_base = rtl_8139_io_base;
	unsigned short status = inw(io_base + OFF_ISR);
	// official documentation says writing to ISR
	// has no effect but it seems to be mandatory
	// to mark the interrupt as handled
	outw(io_base + OFF_ISR, 0xFFFF);
	unsigned short capr = inw(io_base + OFF_CAPR);
	printk("NIC io_base: 0x%x, status 0x%x, CAPR: 0x%x\n", io_base, status, capr);
	pic_eoi(INT_NIC);
}

/*static unsigned short checksum(unsigned short *buf, unsigned int word_count, unsigned int except_word) {
	unsigned short sum = 0;
	for (unsigned int i = 0; i < word_count; i++) {
		if (i == except_word) continue;
		unsigned int tmp = buf[i] + sum;
		sum = tmp & 0xFFFF + (tmp >> 16);
	}
	return sum;
}*/

struct ipv4 {
	unsigned char version_ihl;
	unsigned char dscp_ecn;
	unsigned short total_length;
	unsigned short ident;
	unsigned short flags_frag_offset;
	unsigned char ttl;
	unsigned char prot;
	unsigned short header_sum;
	unsigned char src_ipv4[4];
	unsigned char dst_ipv4[4];
} __attribute__((packed));

struct icmp {
	unsigned short code;
	unsigned short checksum;
	unsigned char data[4];
} __attribute__((packed));

struct ether {
	unsigned char dst_mac[6];
	unsigned char src_mac[6];
	unsigned short ether_type;
} __attribute__((packed));

struct arp_eth_ipv4 {
	unsigned short hardware_type;
	unsigned short protocol_type; // ether type
	unsigned char hw_addr_len;
	unsigned char prot_addr_len;
	unsigned short operation;
	unsigned char sender_mac[6];
	unsigned char sender_ipv4[4];
	unsigned char target_mac[6];
	unsigned char target_ipv4[4];
} __attribute__((packed));

struct icmp_ipv4_frame {
	struct ether ether;
	struct ipv4 ipv4;
	struct icmp icmp;
} __attribute__((packed));

struct arp_ipv4_frame {
	struct ether ether;
	struct arp_eth_ipv4 arp;
} __attribute__((packed));

enum arp_operation {
	// Already in network byte order for little-endian CPU
	ARP_REQUEST = 0x0100,
	ARP_REPLY = 0x0200,
};

enum ether_type {
	// Already in network byte order for little-endian CPU
	ETH_IPV4 = 0x0008,
	ETH_ARP = 0x0608,
};

void rtl_8139_init(unsigned char bus, unsigned char slot) {
		unsigned int io_base = (unsigned int)pci_config_read_word(bus, slot, 0, 18) << 16 | pci_config_read_word(bus, slot, 0, 16);
		if (!(io_base & 1))
			return;

		// Mark as bus master for DMA
		unsigned short command = pci_config_read_word(bus, slot, 0, 4);
		command |= 0x4;
		pci_config_write_word(bus, slot, 0, 4, command);

		io_base = io_base & IO_BAR_MASK;
		rtl_8139_io_base = io_base;
		unsigned char our_mac[] = {inb(io_base), inb(io_base + 1), inb(io_base + 2), inb(io_base + 3), inb(io_base + 4), inb(io_base + 5)};
		printk("MAC: %x:%x:%x:%x:%x:%x\n", our_mac[0], our_mac[1], our_mac[2], our_mac[3], our_mac[4], our_mac[5]);
		outb(io_base + OFF_CONFIG1, 0x0); // LWAKE + LWPTN
		outb(io_base + OFF_CMD, 0x10); // Software reset
		while ((inb(io_base + OFF_CMD) & 0x10) != 0) {}

		unsigned char our_ipv4[] = {192, 168, 76, 9};
		//unsigned char dst_mac[] = {0xd0, 0x46, 0x0c, 0x85, 0xa6, 0x64};
		unsigned char dst_ipv4[] = {192, 168, 76, 2};

		outl(io_base + OFF_RBSTART, (unsigned int)&receive_buffer[0]); // receive buffer start
		outw(io_base + OFF_IMR, 0xffff); // receive all interrupts
		outl(io_base + OFF_TCR, 1 << 15); // Append Ethernet CRC
		outl(io_base + OFF_RCR, 0xf | (1<<7)); // Accept all packets, overflow instead of wrap around
		outb(io_base + OFF_CMD, 0x0c); // Enable receive and transmission

		struct arp_ipv4_frame frame;
		memcpy(frame.ether.dst_mac, "\xff\xff\xff\xff\xff\xff", 6);
		memcpy(frame.ether.src_mac, our_mac, 6);
		frame.ether.ether_type = ETH_ARP;
		frame.arp.hardware_type = 0x0100;
		frame.arp.protocol_type = ETH_IPV4;
		frame.arp.hw_addr_len = 6;
		frame.arp.prot_addr_len = 4;
		memcpy(&frame.arp.sender_mac, our_mac, 6);
		memcpy(&frame.arp.sender_ipv4, our_ipv4, 4);
		memcpy(&frame.arp.target_ipv4, dst_ipv4, 4);
		memset(&frame.arp.target_mac, 0, 6);
		frame.arp.operation = ARP_REQUEST;
		/*struct icmp_ipv4_frame frame;
		memcpy(frame.ether.dst_mac, dst_mac, 6);
		memcpy(frame.ether.src_mac, our_mac, 6);
		frame.ether.ether_type = 0x0080;
		frame.ipv4.version_ihl = 0x54;
		frame.ipv4.total_length = sizeof(struct ipv4) + sizeof(struct icmp);
		frame.ipv4.ident = 0;
		frame.ipv4.ttl = 64;
		frame.ipv4.prot = 1;
		memcpy(frame.ipv4.dst_ipv4, dst_ipv4, 6);
		memcpy(frame.ipv4.src_ipv4, our_ipv4, 6);
		unsigned short ipv4_sum = checksum((unsigned short *)&frame.ipv4, 10, 5);
		unsigned short icmp_sum = checksum((unsigned short *)&frame.icmp, 4, 1);
		frame.ipv4.header_sum = ipv4_sum >> 8;
		frame.ipv4.header_sum = ipv4_sum & 0xFF;
		frame.icmp.checksum = icmp_sum >> 8;
		frame.icmp.checksum = icmp_sum & 0xFF;*/

		memcpy(send_buffer, &frame, sizeof(frame));
		outl(io_base + OFF_TSAD0, (unsigned int)&send_buffer); // transmit start, for now physical = virtual
		outl(io_base + OFF_TSD0, sizeof(frame));
}
