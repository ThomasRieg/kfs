CFLAGS := -m32
ASFLAGS := --32
LDFLAGS := -T link.ld -m elf_i386
SRCS := main.o entry.o
ISO := kfs.iso
ELF := kfs.elf

qemu: $(ISO)
	qemu-system-i386 -cdrom $(ISO)

$(ELF): $(SRCS)
	$(LD) -o $@ $(LDFLAGS) $(SRCS)

$(ISO): $(ELF) grub.cfg
	mkdir -p iso/boot/grub
	cp $(ELF) iso/boot
	cp grub.cfg iso/boot/grub
	grub-mkrescue -o $@ iso
