/**
  Group Led and Designed Operating System (GLaDOS)
  
  A UEFI-based C++ operating system.
*/
#ifndef __GLADOS_GLADOS_H
#define __GLADOS_GLADOS_H

extern "C" {
  // This is the Intel EFI headers.
  //  All the functions are plain C.
  #include <efi/efi.h>
};

/// This is compiled with a Windows-type compiler, so 
///   "long" is only 4 bytes, even on a 64-bit machine.
typedef long long int64_t;
typedef unsigned long long uint64_t;

/// At boot, we define a global variable to store the EFI SystemTable API:
/// See include/efi/efiapi.h for definition of EFI_SYSTEM_TABLE,
///  it's packed with useful stuff.
extern EFI_SYSTEM_TABLE *ST;

/// This macro for errors in this EFI system table function call.
#define UEFI_CHECK(fnCall) check_error(fnCall,#fnCall,__LINE__)
extern void check_error(UINT64 error,const char *function,int line);

/// Erase the screen
extern void clear_screen(void);

/// Print stuff to the console
extern void print(const char *str);
extern void println(const char *str=0); 
extern void print(int value); 
extern void print(int64_t value);
extern void print(uint64_t value);
extern void print_hex(uint64_t value,long digits=16,char separator=' ');

/** Read a Unicode char from the keyboard
   or as a negative number scan code like:
     Up and down arrows: -1 and -2
     Right and left arrows: -3 and -4
     Backspace key: -8
     F1 key: -11
     F9 key: -19
     F10 key: -20
     Escape key: -23
*/
extern int read_char(void);

/// Return true if we should keep scrolling, false if the user hits escape
extern bool pause(void);

/// Turn off interrupts
inline void cli(void) { __asm__("cli"); }
/// Turn on interrupts
inline void sti(void) { __asm__("sti"); }

/// Read and execute user commands 
extern void handle_commands(void);



#endif 

