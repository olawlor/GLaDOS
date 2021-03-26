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
  print_hex((uint64_t)(void *)efi_main);
  print("=efi_main ");

  setup_GDT();
  setup_IDT();
  print("\nBooted OK!\n");
  
  test_graphics();
  
  handle_commands();

  return EFI_SUCCESS;
}

