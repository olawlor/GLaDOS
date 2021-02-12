/*
  Group Led and Designed Operating System (GLaDOS)
  
  A UEFI-based C++ operating system
  by the University of Alaska Fairbanks.
  
  Dr. Orion Lawlor and the UAF CS 321 class, 2021-01 (Public Domain)
*/
#include "GLaDOS/GLaDOS.h"

// We define a global variable to store the EFI SystemTable API:
EFI_SYSTEM_TABLE *ST=0;


// From asm_util.s:
extern "C" uint64_t syscall_setup(); 
extern "C" void syscall_finish(); 
extern "C" uint64_t handle_syscall(uint64_t syscallNumber,uint64_t arg0)
{
    print("  syscall ");
    print((int)syscallNumber);
    if (syscallNumber==1) {
        print("write(");
        int fd=arg0;
        print(fd);
        println(")");
    }
    if (syscallNumber==60) {
        print("exit(");
        int exitcode=arg0;
        print(exitcode);
        println(")");
    }
    return 0;
}


extern "C"
EFI_STATUS
EFIAPI
efi_main (EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
  // Store the SystemTable API into the "ST" global.
  ST=SystemTable;
  
  // Turn off the watchdog, so we can run indefinitely
  ST->BootServices->SetWatchdogTimer(0, 0, 0, (CHAR16 *)NULL);
  
  // Erase the BIOS splash data
  clear_screen();

  // Read and write chars
  println("Hello.  This is GLaDOS, for science.");
  print_hex((uint64_t)(void *)efi_main);
  print("=efi_main ");
  
  // Program p("APPS/PROG"); // <- aspirational interface
  
  // Set up syscalls  
  print("syscalls ");
  syscall_setup();
  
  // Run a program
  FileDataStringSource exe=FileContents("APPS/PROG");
  Byte *code=(Byte *)0x400000;
  ByteBuffer buf; int strIndex=0;
  while (exe.get(buf,strIndex++)) {
     for (char c:buf) {
        *code++ = c;
     }
  }
  // call start
  typedef long (*function_t)(void);
  function_t f=(function_t)0x401000;
  println("Running demo {");
  long ret=f();
  println("}");
  print((int)ret);
  println(" was the return value.");

  syscall_finish();
  
  
  
  handle_commands();

  return EFI_SUCCESS;
}

