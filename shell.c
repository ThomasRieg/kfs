#include "libk/libk.h"
#include "tty/tty.h"
#include "io.h"
#include "pci.h"
#include "net.h"
#include "serial.h"
#include "common.h"
#include "mem_page/mem_paging.h"

void print_clock(void);

void net_test(void) {

	const unsigned char *gateway_mac = arp_lookup(rtl8139.gateway_ipv4);

	///////////////////////////////// ICMP test
	struct icmp_ipv4_frame frame;
	memcpy(frame.ether.dst_mac, gateway_mac, 6);
	memcpy(frame.ether.src_mac, rtl8139.mac, 6);
	frame.ether.ether_type = 0x0080;
	frame.ipv4.version_ihl = 0x54; frame.ipv4.total_length = sizeof(struct ipv4) + sizeof(struct icmp);
	frame.ipv4.ident = 0;
	frame.ipv4.ttl = 64;
	frame.ipv4.prot = 1;
	memcpy(frame.ipv4.dst_ipv4, "\x01\x01\x01\x01", 4); // 1.1.1.1
	memcpy(frame.ipv4.src_ipv4, rtl8139.ipv4, 4);
	unsigned short ipv4_sum = checksum((unsigned short *)&frame.ipv4, 10, 5);
	unsigned short icmp_sum = checksum((unsigned short *)&frame.icmp, 4, 1);
	frame.ipv4.header_sum = ipv4_sum >> 8;
	frame.ipv4.header_sum = ipv4_sum & 0xFF;
	frame.icmp.checksum = icmp_sum >> 8;
	frame.icmp.checksum = icmp_sum & 0xFF;

	//////////////////////////////// TCP test
	/*struct tcp_ipv4_frame frame;
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
	for (int i = 0; i < 3; i++)
		rtl_8139_transmit(&frame, sizeof(frame));
}

void handle_command(unsigned char len, const char *cmd)
{
	bool found = true;

	switch (len)
	{
	case 3:
		if (memcmp(cmd, "pci", 3) == 0)
		{
			pci_enumerate();
		} else if (memcmp(cmd, "net", 3) == 0) {
			net_test();
		} else if (memcmp(cmd, "arp", 3) == 0) {
			arp_print_table();
		} else if (memcmp(cmd, "vga", 3) == 0) {
			for (unsigned int i = 0; i < 16; i++) {
				for (unsigned int j = 0; j < 16; j++) {
					vga_set_color(j, i);
					writes("@");
					vga_set_color(VGA_WHITE, VGA_BLACK);
					writes(" ");
				}
				writes("\n");
			}
			writes("\n");
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
				   "- print-stack: print the stack from top to current esp in hexadecimal\n"
				   "- print-trace: print call backtrace\n"
				   "- crash: crash the kernel by accessing unmapped memory\n"
				   "- clear: clear TTY\n"
				   "- layout: swap layout between QWERTY <-> AZERTY\n"
				   "- breakpoint: cause the breakpoint instruction to be executed\n"
				   "- pci: enumerate all installed PCI devices\n"
				   "- net: test network capabilities\n"
				   "- arp: print ARP table\n"
				   "- vga: print all colour combinations\n"
				   "- fill-memory: memory tester, tries to allocate the entire available memory and memset it to 0\n");
		}
		else if (memcmp(cmd, "date", 4) == 0)
		{
			print_clock();
		}
		else
			found = false;
		break;
	case 5:
		if (memcmp(cmd, "clear", 5) == 0) {
			vga_clear_screen();
			serial_writes("\33[H\33[2J\33[3J");
		}
		else if (memcmp(cmd, "crash", 5) == 0)
		{
			*((unsigned char *)4242424242) = 5;
		}
		else
			found = false;
		break;
	case 6:
		if (memcmp(cmd, "layout", 6) == 0) {
			void tty_swap_layout(void);
			tty_swap_layout();
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
		if (memcmp(cmd, "print-stack", 11) == 0)
		{
			char forty_two[100];
			memset(forty_two, 0x42, 100);
			stack_dump_words(0);
		}
		else if (memcmp(cmd, "print-trace", 11) == 0)
		{
			stack_trace_ebp(32);
		}
		else if (memcmp(cmd, "fill-memory", 11) == 0)
		{
			fill_memory();
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
