CFLAGS := -std=gnu2x -Wall -Wextra -Werror -m32 -MMD -ffreestanding -fno-pie -g
ASFLAGS := --32
LDFLAGS := -T link.ld -m elf_i386
OBJS := main.o ext2.o net.o shell.o entry.o boot_init.o \
		drivers/ps2.o drivers/pci.o drivers/pic.o drivers/rtl8139.o drivers/serial.o drivers/ide.o\
		tty/tty.o\
		vga/vga.o\
		libk/vec.o libk/print_stack.o libk/memstuff.o libk/convertstuff.o libk/strstuff.o libk/printk.o libk/ft_vector.o\
		dt/dt.o\
		mem_page/kmap.o mem_page/paging.o mem_page/utils.o mem_page/kmmap.o mem_page/mem_tester.o\
		pmm/pmm.o\
		vmalloc/init_state.o vmalloc/vcalloc.o vmalloc/vfree.o vmalloc/vmalloc.o vmalloc/vrealloc.o vmalloc/kmalloc.o\
		interrupts/dispatcher.o interrupts/int_entrypoint.o interrupts/isr_stubs.o interrupts/setup_interrupts.o interrupts/handlers/handlers.o\
		syscalls/syscalls.o syscalls/syscall_functions/process_basics.o syscalls/syscall_functions/io.o syscalls/syscall_functions/sys_signals.o\
		tasks/task.o\
		mmap/mmap.o mmap/munmap.o\
		fd/pipe.o fd/inode.o fd/fd_tty.o\
		waitq/waitq.o waitq/sleep.o\
		signals/signals.o signals/signals_asm.o
ISO := kfs.iso
ELF := kfs.elf
DISK_FILE := disk.raw
SHELL := /bin/bash # for brace expansions
GRUB_MKRESCUE := grub-mkrescue
CC := gcc
LD := ld
LIBGCC := $(shell $(CC) -m32 -print-libgcc-file-name) # for 64-bit math

ifeq (, $(shell which $(GRUB_MKRESCUE) 2>/dev/null))
	GRUB_MKRESCUE := grub2-mkrescue
endif

#SERIAL_BACKEND := -chardev socket,path=./serial,server=on,id=char0
SERIAL_BACKEND := -chardev stdio,id=char0

QEMU := qemu-system-i386 $(SERIAL_BACKEND) -serial chardev:char0 -nic none -netdev user,id=net,net=192.168.76.0/24,dhcpstart=192.168.76.9 -device rtl8139,netdev=net -object filter-dump,id=f1,netdev=net,file=netdump.pcap -drive id=iso,file=$(ISO),format=raw -drive id=disk,file=$(DISK_FILE),format=raw -m 4096M

qemu: $(ISO) $(DISK_FILE)
	$(QEMU)

debug: $(ISO) $(DISK_FILE)
	set -m; $(QEMU) -s -S &
	gdb -ix .gdb_init

# yes.
$(DISK_FILE):
	$(MAKE) -C userspace
	mkdir -vp root root/{bin,etc,usr,srv,var,home,root,proc,dev,mnt,run,sys}
	cp userspace/{init,socketpair} root/bin
	i=1; \
	BUSY_BOX_PROGRAMS="ash cat chmod cp date dd df echo hostname kill ln ls mkdir mv ps pwd rm rmdir sleep login hexdump sed sort su tail head tar time whoami vi w wall wc xxd xargs xz users uniq uname less stty find cryptpw loadkmap dumpkmap clear grep tr stat";\
	for bin in $$BUSY_BOX_PROGRAMS; do \
		if [[ ! -x root/bin/$$bin ]]; then\
			echo $$bin \($$i/$$(echo $$BUSY_BOX_PROGRAMS | wc -w)\);\
			curl -o root/bin/$$bin https://www.busybox.net/downloads/binaries/1.35.0-i686-linux-musl/busybox_$$(echo $$bin | tr [:lower:] [:upper:]) ; \
		fi; \
		: $$((i++));\
	done
	cp root/bin/{ash,sh}
	chmod +x root/bin/*
	rm -f $(DISK_FILE) && touch $(DISK_FILE) && fallocate -l 10M $(DISK_FILE) &&\
		parted -s $(DISK_FILE) mklabel msdos mkpart primary ext2 1MiB 100% &&\
		OFFSET=$$(parted -s $(DISK_FILE) unit B print \
  | awk '/^ 1/ { gsub("B","",$$2); print $$2 }') &&\
	  mkfs.ext2 -d root -F -E offset=$$OFFSET $(DISK_FILE) 9M

all: $(ISO)

$(ELF): $(OBJS)
	$(LD) -o $@ $(LDFLAGS) $(OBJS) $(LIBGCC)

$(ISO): $(ELF) grub.cfg
	mkdir -p iso/boot/grub
	cp $(ELF) iso/boot
	cp grub.cfg iso/boot/grub
	$(GRUB_MKRESCUE) -o $@ iso

clean:
	find -name '*.d' -delete
	rm -f $(OBJS)

fclean: clean
	rm -f $(ELF) $(ISO)

re: fclean all

.PHONY: all clean fclean re qemu debug $(DISK_FILE)

-include $(OBJS:%.o=%.d)
