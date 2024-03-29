# Makefile to build an EFI operating system drive image.

# This is a local copy of Intel's EFI API
GNU_EFI_CFLAGS=
# -Iinclude/efi -Iinclude/efi/x86_64

# Warnings and optimization flags
WARNINGS=-Wall 
OPTS=-O1
#OPTS=-g 

# These are the clang compiler flags to build an EFI executable
#   From https://dvdhrm.github.io/2019/01/31/goodbye-gnuefi/
EFI_CFLAGS=-std=c++17 \
        -target x86_64-unknown-windows \
        -ffreestanding \
        -fshort-wchar \
        -fno-stack-protector \
        -mno-red-zone \
	-fno-rtti \
        -Iinclude/efi -Iinclude/efi/x86_64

# These are the ldd flags to emit an EFI executable
EFI_LDFLAGS=-target x86_64-unknown-windows \
        -nostdlib \
        -Wl,-entry:efi_main \
        -Wl,-subsystem:efi_application \
        -fuse-ld=lld-link

CFLAGS=$(WARNINGS) $(OPTS)	$(EFI_CFLAGS) -I.  -Iinclude
LDFLAGS=$(OPTS) $(EFI_LDFLAGS)

# Compile these object files, link the kernel, copy to drive image:
OBJ=libraries/lodepng.o boot.o graphics.o ui.o io.o util.o util_asm.o run_linux.o thread.o

# This is the bootable EFI kernel file
KERNEL=glados.efi

# This is the bootable drive image file.
#  This can be used in virtualbox (VirtualBox/bootdrive.vmdk points to it),
#  or qemu, or it can be copied to a flash drive for use on physical hardware.
DRIVE=my_efi.hdd

# Run the QEMU simulator with these args:
QFLAGS=-L . -drive format=raw,file=$(DRIVE) -m 512  -smp cores=3


# Default build target: bake the full drive image
all: $(DRIVE)


# Here's how we compile each source file:
%.o: %.cpp
	clang -c $< $(CFLAGS) -o $@

%.o: %.s
	nasm -f win64 $< -o $@

# This is the main linker line, to build the actual .efi kernel file:
$(KERNEL): $(OBJ)
	clang $(OBJ)  $(LDFLAGS)  -o $(KERNEL)

# Needs qemu and "bios-256k.bin" from OVMF (EFI firmware).
run: $(DRIVE)
	qemu-system-x86_64 $(QFLAGS)

# Connect gdb debugger with "target remote localhost:1234"
debug: $(DRIVE)
	qemu-system-x86_64 -s $(QFLAGS)

# Disassemble the kernel
dis: $(KERNEL)
	objdump --adjust-vma=0xFFFFFFFEDE7F7000 -M intel -drC $(KERNEL) > dis

dis_nasm: $(KERNEL)
	ndisasm -b 64 $(KERNEL) > dis_nasm


# Userspace programs, for testing
PROGRAMS=APPS/prog
#  APPS/prog_c

# Assembly, making bare linux syscalls
APPS/prog: prog.s
	nasm -f elf64 $< -o prog.o
	ld prog.o -o $@

# Compiled program using the smaller "diet libc":
#   (Currently need to comment out stackgap in config file)
APPS/prog_c: prog_c.c
	/opt/diet/bin/diet gcc -no-pie $< -static -o $@


# This copies the kernel to a FAT16 filesystem on a floppy disk image.
#  Uses mformat (mtools) to avoid needing root access.
$(DRIVE):  $(KERNEL) $(PROGRAMS)
	#dd if=/dev/zero of="$@" bs=1k count=1440
	mformat -i "$@" -f 1440 ::
	mmd -i "$@" ::/EFI
	mmd -i "$@" ::/EFI/BOOT
	mcopy -i "$@" $< ::/EFI/BOOT/BOOTX64.EFI
	mcopy -i "$@" APPS  ::/


# This can make a FAT32 filesystem using Linux kernel calls, 
# but seems to work the same as mformat above.
$(DRIVE)_fat32: $(KERNEL)
	dd if=/dev/zero of=$@ bs=1024k count=40
	- sudo losetup /dev/loop9 $@
	- echo "- - U *" | sudo sfdisk -X gpt /dev/loop9
	sudo partprobe /dev/loop9
	sudo mkfs.fat -F 32 /dev/loop9p1
	sudo mkdir -p tmp
	sudo mount -o loop /dev/loop9p1 tmp
	sudo mkdir -p tmp/EFI/BOOT
	sudo cp $(KERNEL) tmp/EFI/BOOT/BOOTX64.EFI
	sudo cp -r APPS tmp/
	sudo umount tmp
	sudo losetup -d /dev/loop9

clean:
	- rm $(KERNEL) $(OBJ)

apt-get:
	sudo apt-get install build-essential clang lld nasm  mtools qemu-system-x86

