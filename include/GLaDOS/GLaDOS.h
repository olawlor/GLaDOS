/**
  Group Led and Designed Operating System (GLaDOS)
  
  A UEFI-based C++ operating system.
  
  Dr. Orion Lawlor and the UAF CS 321 class, 2021-01 (Public Domain)
*/
#ifndef __GLADOS_GLADOS_H
#define __GLADOS_GLADOS_H

// Paranoia / debugging flags:
#define GLADOS_BOUNDSCHECK 1 /* do bounds checking on array indexes */

#if !GLaDOS_HOSTED
/// This is compiled with a Windows-type compiler, so 
///   "long" is only 4 bytes, even on a 64-bit machine.
typedef long long int64_t;
typedef unsigned long long uint64_t;
#endif

/// Fatal error in kernel: prints the error message and hangs.
void panic(const char *why,uint64_t number=0);

/// Halts forever.  (For example, after a panic.)
void hang(void);

extern "C" {
  // This is the Intel EFI headers.
  //  All the functions there are plain C.
  #include "efi/efi.h"
};


/// At boot, we define a global variable to store the EFI SystemTable API:
/// See include/efi/efiapi.h for definition of EFI_SYSTEM_TABLE,
///  it's packed with useful stuff.
extern EFI_SYSTEM_TABLE *ST;

extern uint64_t trace_code;

/// This macro for errors in this EFI system table function call.
#define UEFI_CHECK(fnCall) check_error(fnCall,#fnCall,__LINE__)
extern void check_error(UINT64 error,const char *function,int line);

/// Widely used utilities (before print and memory allocation)
#include "utility/ByteBuffer.h"
#include "utility/Vector.h"
#include "utility/StringSource.h"

/// Take the square root of this float (GLaDOS version, <math.h> not available here)
extern "C" float sqrtf(float x);

/// Erase the screen
extern void clear_screen(void);

/// Print stuff to the console
extern void print(const char *str);
extern void print(const StringSource &str);
extern void println(void);
extern void println(const StringSource &str); 
extern void print(int value); 
extern void print(int64_t value);
extern void print(uint64_t value);
extern void print_hex(uint64_t value,long digits=16,char separator=' ');

#include "arch/PageTable.h"

// galloc/gfree allocate/deallocate small chunks of memory:
#include "memory/memory.h" // galloc/gfree



/// These are the x86 "in" and "out" I/O instructions.
extern "C" void outportb(int addr,int val);
extern "C" int inportb(int addr);



/** (Blocking) Read a Unicode char from the keyboard
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

/// Reduce idle energy: be kind to the CPU in busy wait
inline void pause_CPU(void) { __asm__("pause"); }

/// Called at startup
extern void setup_GDT(void);
extern void setup_IDT(void);

/// Load and start executing a linux program
extern int run_linux(const char *program_name);

/// Explore the CPU-OS interface data structures
extern void print_idt(void);
extern void test_idt(void);
extern void print_gdt(void);
extern void test_gdt(void);
extern void print_pagetables(void);
extern void test_pagetables(void);
extern void print_graphics(void);
extern void test_graphics(void);
extern void print_threads(void);
extern void test_threads(void);

extern void test_UI(void);

/// Run a "goofy one-char command"
void handle_command(char cmd);

/// Read and execute user commands 
extern void handle_commands(void);



#endif 

