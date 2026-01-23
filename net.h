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

struct icmp_echo {
	unsigned char type;
	unsigned char code;
	unsigned short checksum;
	unsigned short identifier;
	unsigned short sequence_number;
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
	struct icmp_echo icmp;
} __attribute__((packed));

struct arp_ipv4_frame {
	struct ether ether;
	struct arp_eth_ipv4 arp;
} __attribute__((packed));

enum arp_hardware_type {
	// Already in network byte order for little-endian CPU
	ARP_ETHER = 0x0100,
};

enum arp_operation {
	// Already in network byte order for little-endian CPU
	ARP_REQUEST = 0x0100,
	ARP_REPLY = 0x0200,
};

struct arp_eth_ipv4_cache_entry {
	unsigned char ipv4[4];
	unsigned char mac[6];
};

enum ether_type {
	// Already in network byte order for little-endian CPU
	ETH_IPV4 = 0x0008,
	ETH_ARP = 0x0608,
};

struct tcp {
	unsigned short src_port;
	unsigned short destination_port;
	unsigned int sequence_number;
	unsigned int ack_number;
	unsigned short flags;
	unsigned short window;
	unsigned short checksum;
	unsigned short urgent_pointer;
};

struct udp {
	unsigned short src_port;
	unsigned short dst_port;
	unsigned short length;
	unsigned short checksum;
};

struct dns_header {
	unsigned short id;
	unsigned short qr:						1;
	unsigned short opcode:					4;
	unsigned short authoritative_answer:	1;
	unsigned short truncation:				1;
	unsigned short recursion_desired:		1;
	unsigned short recursion_available:		1;
	unsigned short z:						1;
	unsigned short rcode:					4;
	unsigned short question_count;
	unsigned short answer_count;
	unsigned short ns_count;
	unsigned short rr_count;
};

struct dns_question {
	struct dns_header header;
};

struct tcp_ipv4_frame {
	struct ether ether;
	struct ipv4 ipv4;
	struct tcp tcp;
} __attribute__((packed));

struct rtl8139 {
	unsigned int io_base;
	unsigned char receive_buffer[8192 + 16 + 1500];
	unsigned char send_buffers[4][1700];
	unsigned char next_send_buffer;
	unsigned char mac[6];
	unsigned char ipv4[4];
	unsigned char gateway_ipv4[4];
};

extern struct rtl8139 rtl8139;
void rtl_8139_transmit(void *frame, unsigned int size);
unsigned char *arp_lookup(unsigned char ipv4[4]);
void handle_frame(struct ether *ether);
void arp_print_table(void);
unsigned short internet_checksum(unsigned char *buf, unsigned int word_count, unsigned int except_word);
