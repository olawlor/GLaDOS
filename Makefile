# Makefile to build an EFI operating system drive image.

# This is a local copy of Intel's EFI API
GNU_EFI_CFLAGS=-Iinclude/efi -Iinclude/efi/x86_64

WARNINGS=-Wall 
OPTIMIZE=-O1

# These are the clang compiler flags to build an EFI executable
#   From https://dvdhrm.github.io/2019/01/31/goodbye-gnuefi/
EFI_CFLAGS=-target x86_64-unknown-windows \
        -ffreestanding \
        -fshort-wchar \
        -fno-stack-protector -mno-stack-arg-probe \
        -mno-red-zone \
        -Iinclude/efi -Iinclude/efi/x86_64

# These are the ldd flags to emit an EFI executable
EFI_LDFLAGS=-target x86_64-unknown-windows \
        -nostdlib \
        -Wl,-entry:efi_main \
        -Wl,-subsystem:efi_application \
        -fuse-ld=lld-link

CFLAGS=$(WARNINGS) $(OPTIMIZE)	$(EFI_CFLAGS) -I.  -Iinclude
LDFLAGS=$(EFI_LDFLAGS)


OBJ=boot.o io.o util.o
KERNEL=glados.efi
DRIVE=my_efi.hdd


all: run

# Here's how we compile each source file:
boot.o: boot.cpp
	clang $< $(CFLAGS)  -c 

io.o: io.cpp
	clang $< $(CFLAGS)  -c 

util.o: util.cpp
	clang $< $(CFLAGS)  -c 

# This is the main linker line, to build the actual .efi kernel file:
$(KERNEL): $(OBJ)
	clang $(OBJ)  $(LDFLAGS)  -o $(KERNEL)

# Needs qemu and "bios-256k.bin" from OVMF (EFI firmware).
run: $(DRIVE)
	qemu-system-x86_64 -L . -drive format=raw,file=$(DRIVE) -m 512

# This copies the kernel to a FAT16 filesystem on a floppy disk image.
#  Uses mformat (mtools) to avoid needing root access.
$(DRIVE):  $(KERNEL)
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
	- rm $(KERNEL) *.o

apt-get:
	sudo apt-get install build-essential clang lld  mtools  qemu-system-x86

