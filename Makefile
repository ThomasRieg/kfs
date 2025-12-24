CFLAGS := -m32 -MMD -ffreestanding
ASFLAGS := --32
LDFLAGS := -T link.ld -m elf_i386
SRCS := main.o shell.o entry.o pic.o tty/tty.o vga/vga.o libk/print_stack.o libk/memstuff.o libk/convertstuff.o libk/strstuff.o libk/printk.o gdt/gdt.o
ISO := kfs.iso
ELF := kfs.elf

qemu: $(ISO)
	qemu-system-i386 -cdrom $(ISO)

debug: $(ISO)
	qemu-system-i386 -s -S -cdrom $(ISO) &
	gdb -ix .gdb_init

all: $(ISO)

$(ELF): $(SRCS)
	$(LD) -o $@ $(LDFLAGS) $(SRCS)

$(ISO): $(ELF) grub.cfg
	mkdir -p iso/boot/grub
	cp $(ELF) iso/boot
	cp grub.cfg iso/boot/grub
	grub-mkrescue -o $@ iso

clean:
	rm -f $(SRCS) **/*.d

fclean: clean
	rm -f $(ELF) $(ISO)

re: fclean all

.PHONY: all clean fclean re qemu debug

-include *.d
