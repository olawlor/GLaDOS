# GLaDOS
An academic purposes operating system in C++, the 
"Group Led and Designed Operating System". 
Built in C++ on top of EFI, fully 64-bit.

Installation requirements:

	sudo apt-get install build-essentials clang lld  mtools  qemu-system-x86

Needs a recent make, clang, lld, and mtools to build.
This should build inside the Windows Subsystem for Linux,
a Mac UNIX terminal, or a Linux machine. Qemu is just there to run it easily.

To run:

	make run

Reference:

   https://dvdhrm.github.io/2019/01/31/goodbye-gnuefi/
   https://uefi.org/sites/default/files/resources/UEFI%20Spec%202_6.pdf
   https://www.intel.com/content/dam/doc/product-specification/efi-v1-10-specification.pdf
   https://wiki.osdev.org/UEFI
   https://wiki.osdev.org/UEFI_ISO_Bare_Bones
   https://www.rodsbooks.com/efi-programming/hello.html
   https://packages.ubuntu.com/bionic/amd64/gnu-efi/filelist

The name GLaDOS also occurs in the excellent Portal games by Valve.
This university project has no relationship to Valve or Portal.

