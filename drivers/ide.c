#include "pci.h"
#include "../libk/libk.h"

enum ata_reg {
	// which one is accessed depends on CONTROL high bit
	// and whether access is read/write (oof!)
	ATA_REG_DATA       = 0x00,
	ATA_REG_ERROR      = 0x01, // read
	ATA_REG_FEATURES   = 0x01, // write
	ATA_REG_SECCOUNT0  = 0x02,
	ATA_REG_SECCOUNT1  = 0x02,
	ATA_REG_LBA0       = 0x03,
	ATA_REG_LBA3       = 0x03,
	ATA_REG_LBA1       = 0x04,
	ATA_REG_LBA4       = 0x04,
	ATA_REG_LBA2       = 0x05,
	ATA_REG_LBA5       = 0x05,
	ATA_REG_HDDEVSEL   = 0x06,
	ATA_REG_COMMAND    = 0x07, // write
	ATA_REG_STATUS     = 0x07, // read
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

union ata_ident {
	struct {
		unsigned char device_type[2];
		unsigned char cylinders[4];
		unsigned char heads[6];
		unsigned char sectors[8];
		unsigned char serial[34];
		unsigned char model[40];
		unsigned char _whatever1[26];
		unsigned int max_lba;
		unsigned char _whatever2[40];
		unsigned int command_sets;
		unsigned char _whatever3[32];
		unsigned int max_lba_ext;
	} __attribute__((packed));
	unsigned int buf[128];
} __attribute__((packed));

_Static_assert(offsetof(union ata_ident, model) == 54, "incorrect ata_ident struct");
_Static_assert(offsetof(union ata_ident, max_lba) == 120, "incorrect ata_ident struct");
_Static_assert(offsetof(union ata_ident, command_sets) == 164, "incorrect ata_ident struct");
_Static_assert(offsetof(union ata_ident, max_lba_ext) == 200, "incorrect ata_ident struct");

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

				union ata_ident ident;
				for (unsigned short i = 0; i < 128; i++) {
					ident.buf[i] = inl(base + ATA_REG_DATA);
				}
				// model ASCII is packed in low-endian 16-bytes with MSB being first
				for (unsigned short i = 0; i < sizeof(ident.model); i+= 2) {
					char c = ident.model[i + 1];
					if (!c)
						break;
					printk("%c", c);
					printk("%c", ident.model[i]);
				}
				printk("IDE device %u.%u found\n", i, j);
				if (ident.command_sets & (1 << 26)) {
					// 48-bit addressing
					printk("48-bit addressing. sectors: %u, bytes: %u\n", ident.max_lba_ext, ident.max_lba_ext * 512);

					// should read LBA 0 hopefully
					unsigned char lba[512];

					outb(base + ATA_REG_HDDEVSEL, 0xE0);
					outb(base + ATA_REG_SECCOUNT1, 0);

					outb(base + ATA_REG_CONTROL, 0x80);
					outb(base + ATA_REG_LBA3, 0);
					outb(base + ATA_REG_CONTROL, 0x80);
					outb(base + ATA_REG_LBA4, 0);
					outb(base + ATA_REG_CONTROL, 0x80);
					outb(base + ATA_REG_LBA5, 0);

					outb(base + ATA_REG_CONTROL, 0x80);
					outb(base + ATA_REG_SECCOUNT0, 1);

					outb(base + ATA_REG_LBA0, 0);
					outb(base + ATA_REG_LBA1, 0);
					outb(base + ATA_REG_LBA2, 0);

					outb(base + ATA_REG_COMMAND, ATA_CMD_READ_PIO_EXT);
					for (unsigned short i = 0; i < sizeof(lba); i++) {
						lba[i] = inb(base + ATA_REG_DATA);
					}
					hex_dump(lba, sizeof(lba));
				} else {
					// CHS or 28-bit addressing
					printk("28-bit addressing. sectors: %u, bytes %u\n", ident.max_lba, ident.max_lba * 512);
				}
			}
		}
	}
}

void ata_handler(__attribute__((unused)) t_interrupt_data *regs) {
	printk("ATA handler %u called\n", regs->int_no);
}

