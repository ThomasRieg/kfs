#include "io.h"
#include "libk/libk.h"
#include "vga/vga.h"

// See https://wiki.osdev.org/PCI
static unsigned short pci_config_read_word(unsigned char bus, unsigned char slot, unsigned char func, unsigned char offset) {
    unsigned int address;
    unsigned int lbus  = (uint32_t)bus;
    unsigned int lslot = (uint32_t)slot;
    unsigned int lfunc = (uint32_t)func;
    unsigned short tmp = 0;

    // Create configuration address as per Figure 1
    address = (unsigned int)((lbus << 16) | (lslot << 11) |
              (lfunc << 8) | (offset & 0xFC) | ((unsigned int)0x80000000));

    // Write out the address
    outl(PORT_PCI_CONFIG_ADDR, address);
    // Read in the data
    // (offset & 2) * 8) = 0 will choose the first word of the 32-bit register
    tmp = (unsigned short)((inl(PORT_PCI_CONFIG_DATA) >> ((offset & 2) * 8)) & 0xFFFF);
    return tmp;
}

const char *pci_to_name(unsigned short vendor, unsigned short device) {
	if (vendor == 0x10ec && device == 0x8139) {
		return "RTL-8100/8101L/8139 PCI Fast Ethernet Adapter";
	} else if (vendor == 0x8086) {
		if (device == 0x100e) return "82540EM Gigabit Ethernet Controller";
		if (device == 0x7000) return "82371SB PIIX3 ISA";
		if (device == 0x1237) return "440FX - 82441FX PMC [Natoma]";
	}
	return "Unknown";
}

void pci_enumerate(void) {
	const unsigned char bus = 0;
	unsigned char slot = 0;
	printk("PCI devices on bus %u:\n", bus);
	for (;;slot++) {
		unsigned short vendor = pci_config_read_word(bus, slot, 0, 0);
		if (vendor != 0xFFFF) {
			unsigned short device = pci_config_read_word(bus, slot, 0, 2);
			unsigned short class_subclass = pci_config_read_word(bus, slot, 0, 10);
			unsigned char class_code = class_subclass >> 8;
			unsigned char subclass = class_subclass & 0xFF;
			printk("- %x:%x, class %u:%u on slot %u %s\n", vendor, device, class_code, subclass, slot, pci_to_name(vendor, device));
			unsigned short header_type = pci_config_read_word(bus, slot, 0, 14) & 0xFF;
			if (header_type == 0) {
				for (unsigned int i = 0; i < 6; i++) {
					unsigned int bar = (unsigned int)pci_config_read_word(bus, slot, 0, 18 + 4 * i) << 16 | pci_config_read_word(bus, slot, 0, 16 + 4 * i);
					unsigned int base = bar & 1 ? (bar & 0xFFFFFFFC) : (bar & 0xFFFFFFF0);
					vga_set_color(VGA_RED, VGA_BLACK);
					printk("bar%u", i);
					vga_set_color(VGA_WHITE, VGA_BLACK);
					printk(": 0x%x IO: %u ", base, bar & 1);
				}
				writes("\n");
			}
		}

		if (slot == 255)
			break;
	}
}
