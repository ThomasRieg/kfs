#include "pci.h"
#include "io.h"
#include "libk/libk.h"
#include "tty/tty.h"

enum pci_vendor {
	VENDOR_REALTEK = 0x10ec,
	VENDOR_INTEL = 0x8086,
};

enum pci_device {
	DEVICE_REALTEK_RTL8139 = 0x8139,
	DEVICE_INTEL_82540EM = 0x100e,
	DEVICE_INTEL_82371SB = 0x7000,
	DEVICE_INTEL_440FX = 0x1237
};

const char *pci_to_name(unsigned short vendor, unsigned short device) {
	if (vendor == VENDOR_REALTEK && device == DEVICE_REALTEK_RTL8139) {
		return "RTL-8100/8101L/8139 PCI Fast Ethernet Adapter";
	} else if (vendor == VENDOR_INTEL) {
		if (device == DEVICE_INTEL_82540EM) return "82540EM Gigabit Ethernet Controller";
		if (device == DEVICE_INTEL_82371SB) return "82371SB PIIX3 ISA";
		if (device == DEVICE_INTEL_440FX) return "440FX - 82441FX PMC [Natoma]";
	}
	return "Unknown";
}


void rtl_8139_init(unsigned char bus, unsigned char slot);

void pci_init_device(unsigned char bus, unsigned char slot, unsigned short vendor, unsigned short device) {
	if (vendor == VENDOR_REALTEK && device == DEVICE_REALTEK_RTL8139) {
		rtl_8139_init(bus, slot);
	}
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
			vga_set_color(VGA_RED, VGA_BLACK);
			printk("- %x:%x, class %u:%u on slot %u %s\n", vendor, device, class_code, subclass, slot, pci_to_name(vendor, device));
			vga_set_color(VGA_WHITE, VGA_BLACK);
			unsigned short header_type = pci_config_read_word(bus, slot, 0, 14) & 0xFF;
			if (header_type == 0) {
				unsigned short interrupt_line = pci_config_read_word(bus, slot, 0, 0x3C) & 0xFF;
				if (interrupt_line != 0xFF)
					printk("  IRQ: %u\n", interrupt_line);
				printk("  BARs: ");
				for (unsigned int i = 0; i < 6; i++) {
					unsigned int bar = (unsigned int)pci_config_read_word(bus, slot, 0, 18 + 4 * i) << 16 | pci_config_read_word(bus, slot, 0, 16 + 4 * i);
					if (bar) {
						unsigned int base = bar & 1 ? (bar & IO_BAR_MASK) : (bar & 0xFFFFFFF0);
						printk("%u %s 0x%x ", i, bar & 1 ? "IO" : "MM", base);
					}
				}
				writes("\n");
			}
			pci_init_device(bus, slot, vendor, device);
		}

		if (slot == 255)
			break;
	}
}
