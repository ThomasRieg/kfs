#ifndef IDE_H
#define IDE_H
#include "../common.h"

struct ide_drive {
	// IO ports
	unsigned int base;
	unsigned int ctrl;
	bool slave;
};

struct ide_partition {
	struct ide_drive *drive;
	unsigned int first_sector;
	unsigned int sector_count;
};

void ide_read_sector(struct ide_drive *drive, unsigned int lba48, unsigned char buffer[512]);
#endif
