#include "pci.h"
#include "../io.h"
#include "../libk/libk.h"
#include "../tty/tty.h"

enum pci_vendor {
	VENDOR_REALTEK = 0x10ec,
	VENDOR_INTEL = 0x8086,
};

enum pci_device {
	DEVICE_REALTEK_RTL8139 = 0x8139,
	DEVICE_INTEL_82540EM = 0x100e,
	DEVICE_INTEL_82371SB_ISA = 0x7000,
	DEVICE_INTEL_82371SB_IDE = 0x7010,
	DEVICE_INTEL_82371SB_USB = 0x7020,
	DEVICE_INTEL_82371_ACPI = 0x7113,
	DEVICE_INTEL_440FX = 0x1237
};

const char *pci_to_name(unsigned short vendor, unsigned short device) {
	if (vendor == VENDOR_REALTEK && device == DEVICE_REALTEK_RTL8139) {
		return "RTL-8100/8101L/8139 PCI Fast Ethernet Adapter";
	} else if (vendor == VENDOR_INTEL) {
		if (device == DEVICE_INTEL_82540EM) return "82540EM Gigabit Ethernet Controller";
		if (device == DEVICE_INTEL_82371SB_ISA) return "82371SB PIIX3 ISA";
		if (device == DEVICE_INTEL_82371SB_IDE) return "82371SB PIIX3 IDE";
		if (device == DEVICE_INTEL_82371SB_USB) return "82371SB PIIX3 USB";
		if (device == DEVICE_INTEL_82371_ACPI) return "82371AB/EB/MB PIIX4 ACPI";
		if (device == DEVICE_INTEL_440FX) return "440FX - 82441FX PMC [Natoma]";
	}
	return "Unknown";
}

struct pci_installed pci_devices[100];
unsigned int pci_device_count = 0;

void rtl_8139_init(struct pci_installed *installed);

void pci_init_device(struct pci_installed *installed) {
	if (installed->vendor == VENDOR_REALTEK && installed->device == DEVICE_REALTEK_RTL8139) {
		rtl_8139_init(installed);
	}
}

void pci_init_all(void) {
	unsigned char bus = 0;
	for (;;bus++) {
		unsigned char slot = 0;
		for (;;slot++) {
			unsigned char function = 0;
			for (;;function++) {
				unsigned short vendor = pci_config_read_word(bus, slot, function, 0);
				if (vendor != 0xFFFF) {
					struct pci_installed *installed = &pci_devices[pci_device_count++];
					installed->vendor = vendor;
					installed->device = pci_config_read_word(bus, slot, function, 2);
					installed->bus = bus;
					installed->slot = slot;
					installed->function = function;

					unsigned short class_subclass = pci_config_read_word(bus, slot, function, 10);
					installed->class_code = class_subclass >> 8;
					installed->subclass = class_subclass & 0xFF;
					installed->header_type = pci_config_read_word(bus, slot, function, 14) & 0xFF;
					if (installed->header_type == 0) {
						installed->irq = pci_config_read_word(bus, slot, function, 0x3C) & 0xFF;
						for (unsigned int i = 0; i < 6; i++) {
							installed->bars[i] = (unsigned int)pci_config_read_word(bus, slot, function, 18 + 4 * i) << 16 | pci_config_read_word(bus, slot, function, 16 + 4 * i);
						}
					}
					pci_init_device(installed);
				}
				if (function == 7)
					break;
			}

			if (slot == 31)
				break;
		}
		if (bus == 255)
			break;
	}
}

void pci_enumerate(void) {
	printk("PCI devices:\n");
	for (unsigned int i = 0; i < pci_device_count; i++) {
		struct pci_installed *installed = &pci_devices[i];
		vga_set_color(VGA_RED, VGA_BLACK);
		printk("- %u:%u.%u %x:%x, class %u:%u %s\n", installed->bus, installed->slot, installed->function, installed->vendor, installed->device, installed->class_code, installed->subclass, pci_to_name(installed->vendor, installed->device));
		vga_set_color(VGA_WHITE, VGA_BLACK);
		if (installed->header_type == 0) {
			unsigned short interrupt_line = installed->irq;
			if (interrupt_line != 0xFF)
				printk("  IRQ: %u\n", interrupt_line);
			printk("  BARs: ");
			for (unsigned int i = 0; i < 6; i++) {
				unsigned int bar = installed->bars[i];
				if (bar) {
					unsigned int base = bar & 1 ? (bar & IO_BAR_MASK) : (bar & 0xFFFFFFF0);
					printk("%u %s 0x%x ", i, bar & 1 ? "IO" : "MM", base);
				}
			}
			writes("\n");
		}
	}
}
