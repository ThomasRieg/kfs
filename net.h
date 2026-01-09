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

enum arp_hardware_type {
	// Already in network byte order for little-endian CPU
	ARP_ETHER = 0x0100,
};

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

