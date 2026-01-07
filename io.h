#define PIC_OFFSET 32

enum port {
	PORT_CMOS_INDEX = 0x70,
	PORT_CMOS_DATA = 0x71,
	PORT_VGA_INDEX = 0x3d4,
	PORT_VGA_INDEXED = 0x3d5,
	PORT_PS2_DATA = 0x60,
	PORT_PIC_PRIMARY_CMD = 0x20,
	PORT_PIC_PRIMARY_DATA = 0x21,
	PORT_PIC_SECONDARY_CMD = 0xA0,
	PORT_PIC_SECONDARY_DATA = 0xA1,
	PORT_PCI_CONFIG_ADDR = 0xCF8,
	PORT_PCI_CONFIG_DATA = 0xCFC,
};

static inline void outb(unsigned short port, unsigned char value) {
	asm volatile("outb %b0, %w1": : "a" (value), "Nd"(port) : "memory");
}

static inline void outw(unsigned short port, unsigned short value) {
	asm volatile("outw %w0, %w1": : "a" (value), "Nd"(port) : "memory");
}

static inline unsigned char inb(unsigned short port)
{
    unsigned char ret;
    __asm__ volatile ( "inb %w1, %b0"
                   : "=a"(ret)
                   : "Nd"(port)
                   : "memory");
    return ret;
}

static inline void outl(unsigned short port, unsigned int value) {
	asm volatile("outl %0, %w1": : "a" (value), "Nd"(port) : "memory");
}

static inline unsigned int inl(unsigned short port)
{
    unsigned int ret;
    __asm__ volatile ( "inl %w1, %0"
                   : "=a"(ret)
                   : "Nd"(port)
                   : "memory");
    return ret;
}

static inline void io_wait(void)
{
	outb(0x80, 0);
}
