/*
  Group Led and Designed Operating System (GLaDOS)
  
  A UEFI-based C++ operating system
  by the University of Alaska Fairbanks.
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
  print_hex(0xc0de600d);
  
  handle_commands();

  return EFI_SUCCESS;
}

