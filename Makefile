CFLAGS := -m32
ASFLAGS := --32
LDFLAGS := -T link.ld -m elf_i386
SRCS := main.o entry.o

kernel: $(SRCS)
	ld -o kernel $(LDFLAGS) $(SRCS)
