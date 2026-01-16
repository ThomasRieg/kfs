#include "pci.h"
#include "../libk/libk.h"

enum ata_reg {
	ATA_REG_DATA       = 0x00,
	ATA_REG_ERROR      = 0x01, // read
	ATA_REG_FEATURES   = 0x01, // write
	ATA_REG_SECCOUNT0  = 0x02,
	ATA_REG_LBA0       = 0x03,
	ATA_REG_LBA1       = 0x04,
	ATA_REG_LBA2       = 0x05,
	ATA_REG_HDDEVSEL   = 0x06,
	ATA_REG_COMMAND    = 0x07, // write
	ATA_REG_STATUS     = 0x07, // read
	ATA_REG_SECCOUNT1  = 0x08,
	ATA_REG_LBA3       = 0x09,
	ATA_REG_LBA4       = 0x0A,
	ATA_REG_LBA5       = 0x0B,
	ATA_REG_DEVADDRESS = 0x0D
};

enum ata_reg_ctrl {
	ATA_REG_CONTROL    = 0x02, // write
	ATA_REG_ALTSTATUS  = 0x02, // read
};

enum ata_cmd {
	ATA_CMD_READ_PIO          = 0x20,
	ATA_CMD_READ_PIO_EXT      = 0x24,
	ATA_CMD_READ_DMA          = 0xC8,
	ATA_CMD_READ_DMA_EXT      = 0x25,
	ATA_CMD_WRITE_PIO         = 0x30,
	ATA_CMD_WRITE_PIO_EXT     = 0x34,
	ATA_CMD_WRITE_DMA         = 0xCA,
	ATA_CMD_WRITE_DMA_EXT     = 0x35,
	ATA_CMD_CACHE_FLUSH       = 0xE7,
	ATA_CMD_CACHE_FLUSH_EXT   = 0xEA,
	ATA_CMD_PACKET            = 0xA0,
	ATA_CMD_IDENTIFY_PACKET   = 0xA1,
	ATA_CMD_IDENTIFY          = 0xEC
};

void ide_init(struct pci_installed *installed) {
	if (installed->prog_if & 1) {
		printk("IDE controller not in compatibility mode!!\n");
	} else {
		unsigned int base_primary_channel = 0x1F0;
		unsigned int base_primary_channel_ctrl = 0x3F6;
		unsigned int base_secondary_channel = 0x170;
		unsigned int base_secondary_channel_ctrl = 0x376;
		for (unsigned int i = 0; i < 2; i++) {
			unsigned int base = i == 0 ? base_primary_channel : base_secondary_channel;
			unsigned int ctrl = i == 0 ? base_primary_channel_ctrl : base_secondary_channel_ctrl;
			outb(ctrl + ATA_REG_CONTROL, 2); // disable IRQs
			for (unsigned short j = 0; j < 2; j++) {
				outb(base + ATA_REG_HDDEVSEL, 0xA0 | (j << 4));
				outb(base + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
				if (inb(base + ATA_REG_STATUS) == 0)
					continue;
				unsigned int buf[128];
				for (unsigned short i = 0; i < 128; i++) {
					buf[i] = inl(base + ATA_REG_DATA);
				}
				for (unsigned short i = 54; i < 54 + 40; i+= 2) {
					char c = ((unsigned char *)buf)[i + 1];
					if (!c)
						break;
					printk("%c", c);
					printk("%c", ((unsigned char *)buf)[i]);
				}
				printk("IDE device %u.%u found\n", i, j);
			}
		}
	}
}

void ata_handler(__attribute__((unused)) t_interrupt_data *regs) {
	printk("ATA handler %u called\n", regs->int_no);
}

