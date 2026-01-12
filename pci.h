#ifndef PCI_H
#define PCI_H

#include "common.h"
#include "io.h"

#define IO_BAR_MASK 0xFFFFFFFC

// See https://wiki.osdev.org/PCI
static inline unsigned short pci_config_read_word(unsigned char bus, unsigned char slot, unsigned char func, unsigned char offset) {
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

static inline void pci_config_write_word(unsigned char bus, unsigned char slot, unsigned char func, unsigned char offset, unsigned short value) {
	unsigned int address;
	unsigned int lbus  = (uint32_t)bus;
	unsigned int lslot = (uint32_t)slot;
	unsigned int lfunc = (uint32_t)func;

	// Create configuration address as per Figure 1
	address = (unsigned int)((lbus << 16) | (lslot << 11) |
			  (lfunc << 8) | (offset & 0xFC) | ((unsigned int)0x80000000));

	// Write out the address
	outl(PORT_PCI_CONFIG_ADDR, address);
	unsigned short tmp = inl(PORT_PCI_CONFIG_DATA);
	tmp &= 0xFFFF << (!(offset & 2) * 8);
	tmp |= value << ((offset & 2) * 8);
	outl(PORT_PCI_CONFIG_DATA, tmp);
}

void pci_enumerate(void);
#endif
