struct elf_header {
	unsigned char signature[4];
	unsigned char bits;
	unsigned char endianness;
	unsigned char header_version;
	unsigned char abi;
	unsigned char abi_version;
	unsigned char _padding[6];
	unsigned char id_size;
	unsigned short type;
	unsigned short target;
	unsigned int version;
	unsigned int entrypoint;
	unsigned int program_hdrs_offset;
	unsigned int section_hdrs_offset;
	unsigned int flags;
	unsigned short header_size;
	unsigned short program_header_size;
	unsigned short program_header_count;
	unsigned short section_header_size;
	unsigned short section_header_count;
	unsigned short symbol_section_index;
};

struct elf_program_header {
	unsigned int type;
	unsigned int file_offset;
	unsigned int virt_addr;
	unsigned int phys_addr;
	unsigned int file_size;
	unsigned int mem_size;
	unsigned int flags;
	unsigned int align;
};

