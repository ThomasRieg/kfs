#include "common.h"
#include "net.h"
#include "libk/libk.h"

struct arp_eth_ipv4_cache_entry arp_cache[10];
unsigned int arp_cache_entry_count = 0;

unsigned short internet_checksum(unsigned char *buf, unsigned int word_count, unsigned int except_word) {
	unsigned short sum = 0;
	for (unsigned int i = 0, j = 0; i < word_count; i++, j += 2) {
		if (i == except_word) continue;
		unsigned int tmp = ((unsigned int)buf[j] << 8) + buf[j + 1] + sum;
		tmp = (tmp & 0xFFFF) + (tmp >> 16);
		sum = (tmp & 0xFFFF) + (tmp >> 16);
	}
	return ~sum;
}

static void arp_update_cache(struct arp_eth_ipv4 *arp) {
	if (arp->operation == ARP_REPLY) {
		unsigned int i = 0;
		for (; i < arp_cache_entry_count; i++) {
			// update existing entry
			if (memcmp(&arp_cache[i].ipv4, arp->sender_ipv4, 4) == 0) {
				memcpy(&arp_cache[i].mac, arp->sender_mac, 6);
				return;
			}
		}
		// new entry
		if (arp_cache_entry_count < sizeof(arp_cache)/sizeof(arp_cache[0])) {
			struct arp_eth_ipv4_cache_entry *entry = arp_cache + arp_cache_entry_count++;
			memcpy(&entry->ipv4, arp->sender_ipv4, 4);
			memcpy(&entry->mac, arp->sender_mac, 6);
		}
	}
}

void arp_print_table(void) {
	printk("ARP entries (%u/10):\n", arp_cache_entry_count);
	for (unsigned int i = 0; i < arp_cache_entry_count; i++) {
		printk("%u. %u.%u.%u.%u -> %x:%x:%x:%x:%x:%x\n", i,
			arp_cache[i].ipv4[0],
			arp_cache[i].ipv4[1],
			arp_cache[i].ipv4[2],
			arp_cache[i].ipv4[3],
			arp_cache[i].mac[0],
			arp_cache[i].mac[1],
			arp_cache[i].mac[2],
			arp_cache[i].mac[3],
			arp_cache[i].mac[4],
			arp_cache[i].mac[5]);
	}
}

unsigned char *arp_lookup(unsigned char ipv4[4]) {
	for (int i = 0; i < 4; i++) {
		for (unsigned int j = 0; j < arp_cache_entry_count; j++) {
			printk("%u\n", j);
			if (memcmp(ipv4, arp_cache[j].ipv4, 4) == 0)
				return arp_cache[j].mac;
		}

		// send ARP packet
		struct arp_ipv4_frame frame;
		memcpy(frame.ether.dst_mac, "\xff\xff\xff\xff\xff\xff", 6);
		memcpy(frame.ether.src_mac, rtl8139.mac, 6);
		frame.ether.ether_type = ETH_ARP;
		frame.arp.hardware_type = ARP_ETHER;
		frame.arp.protocol_type = ETH_IPV4;
		frame.arp.hw_addr_len = 6;
		frame.arp.prot_addr_len = 4;
		memcpy(&frame.arp.sender_mac, rtl8139.mac, 6);
		memcpy(&frame.arp.sender_ipv4, rtl8139.ipv4, 4);
		memcpy(&frame.arp.target_ipv4, ipv4, 4);
		memset(&frame.arp.target_mac, 0, 6);
		frame.arp.operation = ARP_REQUEST;
		rtl_8139_transmit(&frame, sizeof(frame));

		// waste some cycles and hope we get a reply before then
		for (int z = 0; z < 100000000; z++) {}
	}
	return NULL;
}

void handle_frame(struct ether *ether) {
	unsigned char *d = &ether->dst_mac[0];
	unsigned char *s = &ether->src_mac[0];
	printk("received eth type %x dst %x:%x:%x:%x:%x:%x src %x:%x:%x:%x:%x:%x\n",
		   ether->ether_type,
		   d[0], d[1], d[2], d[3], d[4], d[5],
		   s[0], s[1], s[2], s[3], s[4], s[5]);

	if (ether->ether_type == ETH_ARP)
	{
		struct arp_eth_ipv4 *arp = (struct arp_eth_ipv4 *)(ether + 1);
		if (arp->hardware_type == ARP_ETHER && arp->protocol_type == ETH_IPV4 && arp->prot_addr_len == 4 && arp->hw_addr_len == 6)
		{
			printk("ARP %s sender %u.%u.%u.%u %x:%x:%x:%x:%x:%x target %u.%u.%u.%u %x:%x:%x:%x:%x:%x\n", arp->operation == ARP_REQUEST ? "request" : "reply",
				   arp->sender_ipv4[0], arp->sender_ipv4[1], arp->sender_ipv4[2], arp->sender_ipv4[3],
				   arp->sender_mac[0], arp->sender_mac[1], arp->sender_mac[2], arp->sender_mac[3], arp->sender_mac[4], arp->sender_mac[5],
				   arp->target_ipv4[0], arp->target_ipv4[1], arp->target_ipv4[2], arp->target_ipv4[3],
				   arp->target_mac[0], arp->target_mac[1], arp->target_mac[2], arp->target_mac[3], arp->target_mac[4], arp->target_mac[5]

			);
			arp_update_cache(arp);
		}
	}
}
