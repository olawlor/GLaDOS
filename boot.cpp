/*
  Group Led and Designed Operating System (GLaDOS)
  
  A UEFI-based C++ operating system
  by the University of Alaska Fairbanks.
  
  Dr. Orion Lawlor and the UAF CS 321 class, 2021-01 (Public Domain)
*/
#include "GLaDOS/GLaDOS.h"



// We define a global variable to store the EFI SystemTable API:
EFI_SYSTEM_TABLE *ST=0;

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
  //print_hex(0xc0de600d);
  /*
  for (int i=0;i<100000000;i++) {
      int *ptr=new int[1000]; // (int *)galloc(sizeof(int));
      *ptr=3+i;
      
      if (i%1000 == 0) {
          print("We just allocated ");
          print(sizeof(int));
          print(" bytes at ");
          print_hex((uint64_t)ptr);
          
          print("Read back: ");
          print(*ptr);
          
          println();
      }
      delete[] ptr;
  }
  */
  
  handle_commands();

  return EFI_SUCCESS;
}

