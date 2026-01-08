CFLAGS := -Wall -Wextra -Werror -m32 -MMD -ffreestanding -g
ASFLAGS := --32
LDFLAGS := -T link.ld -m elf_i386
SRCS := main.o shell.o entry.o pic.o tty/tty.o vga/vga.o libk/print_stack.o libk/memstuff.o libk/convertstuff.o libk/strstuff.o libk/printk.o gdt/gdt.o mem_page/kmap.o mem_page/paging.o mem_page/utils.o mem_page/kmmap.o mem_page/mem_tester.o pmm/pmm.o pci.o vmalloc/init_state.o vmalloc/vcalloc.o vmalloc/vfree.o vmalloc/vmalloc.o vmalloc/vrealloc.o
ISO := kfs.iso
ELF := kfs.elf

QEMU := qemu-system-i386 -chardev stdio,id=char0 -serial chardev:char0 -device rtl8139 -cdrom $(ISO) -m 128M

qemu: $(ISO)
	$(QEMU)

debug: $(ISO)
	set -m; $(QEMU) -s -S &
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

-include **/*.d
