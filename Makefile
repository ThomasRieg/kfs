CFLAGS := -std=gnu2x -Wall -Wextra -Werror -m32 -MMD -ffreestanding -g
ASFLAGS := --32
LDFLAGS := -T link.ld -m elf_i386
OBJS := main.o ext2.o net.o shell.o entry.o boot_init.o \
		drivers/ps2.o drivers/pci.o drivers/pic.o drivers/rtl8139.o drivers/serial.o drivers/ide.o\
		tty/tty.o\
		vga/vga.o\
		libk/print_stack.o libk/memstuff.o libk/convertstuff.o libk/strstuff.o libk/printk.o libk/ft_vector.o\
		gdt/gdt.o\
		mem_page/kmap.o mem_page/paging.o mem_page/utils.o mem_page/kmmap.o mem_page/mem_tester.o\
		pmm/pmm.o\
		vmalloc/init_state.o vmalloc/vcalloc.o vmalloc/vfree.o vmalloc/vmalloc.o vmalloc/vrealloc.o vmalloc/kmalloc.o\
		interrupts/dispatcher.o interrupts/int_entrypoint.o interrupts/isr_stubs.o interrupts/setup_interrupts.o interrupts/handlers/handlers.o\
		syscalls/syscalls.o syscalls/syscall_functions/process_basics.o\
		tasks/task.o\
		mmap/mmap.o
ISO := kfs.iso
ELF := kfs.elf
DISK_FILE := disk.raw
SHELL := /bin/bash # for brace expansions

QEMU := qemu-system-i386 -chardev stdio,id=char0 -serial chardev:char0 -nic none -netdev user,id=net,net=192.168.76.0/24,dhcpstart=192.168.76.9 -device rtl8139,netdev=net -object filter-dump,id=f1,netdev=net,file=netdump.pcap -drive id=iso,file=$(ISO),format=raw -drive id=disk,file=$(DISK_FILE),format=raw -m 4096M

qemu: $(ISO) $(DISK_FILE)
	$(QEMU)

debug: $(ISO) $(DISK_FILE)
	set -m; $(QEMU) -s -S &
	gdb -ix .gdb_init

# yes.
$(DISK_FILE):
	rm -rf root
	mkdir -vp root root/{etc,usr,srv,var,home,root,proc,dev,mnt,run,sys}
	rm -f $(DISK_FILE) && touch $(DISK_FILE) && fallocate -l 10M $(DISK_FILE) &&\
		parted -s $(DISK_FILE) mklabel msdos mkpart primary ext2 1MiB 100% &&\
		OFFSET=$$(parted -s $(DISK_FILE) unit B print \
  | awk '/^ 1/ { gsub("B","",$$2); print $$2 }') &&\
		mkfs.ext2 -d root -F -E offset=$$OFFSET $(DISK_FILE) 9M

all: $(ISO)

$(ELF): $(OBJS)
	$(LD) -o $@ $(LDFLAGS) $(OBJS)

$(ISO): $(ELF) grub.cfg
	mkdir -p iso/boot/grub
	cp $(ELF) iso/boot
	cp grub.cfg iso/boot/grub
	grub-mkrescue -o $@ iso

clean:
	find -name '*.d' -delete
	rm -f $(OBJS)

fclean: clean
	rm -f $(ELF) $(ISO)

re: fclean all

.PHONY: all clean fclean re qemu debug $(DISK_FILE)

-include $(OBJS:%.o=%.d)
