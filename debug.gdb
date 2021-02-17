set disassembly-flavor intel
add-symbol-file glados.efi 0x1E7F9000
target remote | qemu-system-x86_64 -S -gdb stdio -L . -drive format=raw,file=my_efi.hdd -m 512

