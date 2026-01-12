CFLAGS := -Wall -Wextra -Werror -m32 -MMD -ffreestanding -g
ASFLAGS := --32
LDFLAGS := -T link.ld -m elf_i386
SRCS := main.o shell.o entry.o pic.o rtl8139.o tty/tty.o vga/vga.o libk/print_stack.o libk/memstuff.o libk/convertstuff.o libk/strstuff.o libk/printk.o gdt/gdt.o mem_page/kmap.o mem_page/paging.o mem_page/utils.o mem_page/kmmap.o mem_page/mem_tester.o pmm/pmm.o pci.o vmalloc/init_state.o vmalloc/vcalloc.o vmalloc/vfree.o vmalloc/vmalloc.o vmalloc/vrealloc.o vmalloc/kmalloc.o interupts/dispatcher.o interupts/int_entrypoint.o interupts/isr_stubs.o interupts/setup_interupts.o interupts/handlers/handlers.o
ISO := kfs.iso
ELF := kfs.elf

QEMU := qemu-system-i386 -chardev stdio,id=char0 -serial chardev:char0 -nic none -netdev user,id=net,net=192.168.76.0/24,dhcpstart=192.168.76.9 -device rtl8139,netdev=net -object filter-dump,id=f1,netdev=net,file=netdump.pcap -cdrom $(ISO) -m 128M

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
