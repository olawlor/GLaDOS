# GLaDOS
An academic purposes operating system in C++, the 
"Group Led and Designed Operating System". 
Built in C++ on top of EFI, fully 64-bit.

Installation requirements:

	sudo apt-get install build-essential clang lld nasm mtools  qemu-system-x86

Needs a recent make, clang, lld, and mtools to build.
This should build on a Linux machine, inside the 
Windows Subsystem for Linux, or in a Mac UNIX terminal 
(untested!).

The build process is that clang bakes the source files into "glados.efi".
Then mtools copies this bootable kernel image to the FAT partition 
in "my_efi.hdd" in the EFI boot path "/EFI/BOOT/BOOTX64.EFI".

Qemu is just there to run it easily from the command line.
The VirtualBox/ directory has a VMDK file that points to "my_efi.hdd".


To run:
	make run

Reference:
   https://www.cs.uaf.edu/2021/spring/cs321/
   https://dvdhrm.github.io/2019/01/31/goodbye-gnuefi/
   https://uefi.org/sites/default/files/resources/UEFI%20Spec%202_6.pdf
   https://www.intel.com/content/dam/doc/product-specification/efi-v1-10-specification.pdf
   https://wiki.osdev.org/UEFI
   https://wiki.osdev.org/UEFI_ISO_Bare_Bones
   https://www.rodsbooks.com/efi-programming/hello.html
   https://packages.ubuntu.com/bionic/amd64/gnu-efi/filelist

The name GLaDOS also occurs in the excellent Portal games by Valve.
This university project has no relationship to Valve or Portal.

