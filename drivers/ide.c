#include "pci.h"
#include "../tty/tty.h"
#include "../libk/libk.h"
#include "ide.h"
#include "../ext2.h"

enum ata_reg {
	// port offsets of ATA registers
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
	ATA_REG_CONTROL    = 0x00, // write
	ATA_REG_ALTSTATUS  = 0x00, // read
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

// Status Register bits
#define ATA_SR_BSY     0x80    // Busy
#define ATA_SR_DRDY    0x40    // Drive ready
#define ATA_SR_DF      0x20    // Drive write fault
#define ATA_SR_DSC     0x10    // Drive seek complete
#define ATA_SR_DRQ     0x08    // Data request ready
#define ATA_SR_CORR    0x04    // Corrected data
#define ATA_SR_IDX     0x02    // Index
#define ATA_SR_ERR     0x01    // Error

// Error Register bits, see below for descriptions
#define ATA_ER_BBK      0x80
#define ATA_ER_UNC      0x40
#define ATA_ER_MC       0x20
#define ATA_ER_IDNF     0x10
#define ATA_ER_MCR      0x08
#define ATA_ER_ABRT     0x04
#define ATA_ER_TK0NF    0x02
#define ATA_ER_AMNF     0x01

static inline void print_ata_error(unsigned char error) {
	writes("error while accessing ATA drive: ");
	if (error & ATA_ER_BBK) writes("Bad block");
	if (error & ATA_ER_UNC) writes("Uncorrectable data");
	if (error & ATA_ER_MC) writes("Media changed");
	if (error & ATA_ER_IDNF) writes("ID mark not found");
	if (error & ATA_ER_MCR) writes("Media change request");
	if (error & ATA_ER_ABRT) writes("Command aborted");
	if (error & ATA_ER_TK0NF) writes("Track 0 not found");
	if (error & ATA_ER_AMNF) writes("No address mark");
	writes("\n");
}

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

void ide_read_sector(struct ide_drive *drive, unsigned int lba48, unsigned char buffer[512]) {
	unsigned int base = drive->base;
	outb(base + ATA_REG_HDDEVSEL, 0xE0 | (drive->slave << 4));

	outb(base + ATA_REG_SECCOUNT1, 0);
	outb(base + ATA_REG_LBA3, (lba48 & 0xFF000000) >> 24);
	outb(base + ATA_REG_LBA4, 0);
	outb(base + ATA_REG_LBA5, 0);

	outb(base + ATA_REG_SECCOUNT0, 1);

	outb(base + ATA_REG_LBA0, lba48 & 0xFF);
	outb(base + ATA_REG_LBA1, (lba48 & 0xFF00) >> 8);
	outb(base + ATA_REG_LBA2, (lba48 & 0xFF0000) >> 16);

	outb(base + ATA_REG_COMMAND, ATA_CMD_READ_PIO_EXT);
	while (1) {
		unsigned char status = inb(base + ATA_REG_STATUS);
		if (status & ATA_SR_ERR) {
			unsigned char error = inb(base + ATA_REG_ERROR);
			print_ata_error(error);
			return;
		}
		if (!(status & ATA_SR_BSY) && (status & ATA_SR_DRQ)) break;
	}

	for (unsigned short i = 0; i < 512; i += 2) {
		unsigned short word = inw(base + ATA_REG_DATA);
		buffer[i] = word & 0xFF;
		buffer[i + 1] = word >> 8;
	}
}

struct mbr_partition {
	unsigned char boot_indicator;
	unsigned char starting_head;
	unsigned short starting_sector: 6;
	unsigned short starting_cylinder: 10;
	unsigned char system_id;
	unsigned char ending_head;
	unsigned short ending_sector: 6;
	unsigned short ending_cylnder: 10;
	unsigned int relative_selector;
	unsigned int total_sectors;
} __attribute__((packed));

enum partition_type {
	PART_LINUX = 0x83
};

const char *partition_type_str(unsigned char type) {
	if (type == 0xEE)
		return "GPT Protective MBR";
	else if (type == 0x0b)
		return "32-bit FAT";
	else if (type == 0x82)
		return "Linux swap";
	else if (type == PART_LINUX)
		return "Linux native";
	return "unknown";
}

void ide_init(struct pci_installed *installed) {
	if (installed->prog_if & 1) {
		printk("IDE controller not in compatibility mode!!\n");
	} else {
		unsigned int base_primary_channel = 0x1F0;
		unsigned int base_primary_channel_ctrl = 0x3F6;
		unsigned int base_secondary_channel = 0x170;
		unsigned int base_secondary_channel_ctrl = 0x376;
		struct ide_drive drive;
		for (unsigned int i = 0; i < 2; i++) {
			unsigned int base = i == 0 ? base_primary_channel : base_secondary_channel;
			unsigned int ctrl = i == 0 ? base_primary_channel_ctrl : base_secondary_channel_ctrl;
			drive.base = base;
			drive.ctrl = ctrl;
			outb(ctrl + ATA_REG_CONTROL, 2); // disable IRQs
			for (unsigned short j = 0; j < 2; j++) {
				drive.slave = j;
				outb(base + ATA_REG_HDDEVSEL, 0xA0 | (j << 4));
				outb(base + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
				if (inb(base + ATA_REG_STATUS) == 0)
					continue;

				while (1) {
					unsigned char status = inb(base + ATA_REG_STATUS);
					if (status & ATA_SR_ERR) {
						unsigned char error = inb(base + ATA_REG_ERROR);
						print_ata_error(error);
						break;
					}
					if (!(status & ATA_SR_BSY) && (status & ATA_SR_DRQ)) break;
				}

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

					unsigned char sector[512];
					ide_read_sector(&drive, 0, sector);
					if (sector[510] == 0x55 && sector[511] == 0xAA) {
						// probably MBR
						struct mbr_partition *partition = (struct mbr_partition *)&sector[0x1BE];
						for (unsigned int i = 0; i < 4; i++) {
							unsigned short type = partition[i].system_id;
							if (type == 0)
								continue;
							printk("partition %u: type %u %s, total sectors %u\n", i, type, partition_type_str(type), partition[i].total_sectors);
							if (type == PART_LINUX) {
								ext2(&drive, partition[i].relative_selector, partition[i].total_sectors);
							}
						}
					}
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

