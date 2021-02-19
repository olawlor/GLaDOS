/*
  Utility functions for GLaDOS
  
  Dr. Orion Lawlor and the UAF CS 321 class, 2021-01 (Public Domain)
*/

/* make headers actually include function definitions here */
#define GLaDOS_IMPLEMENT_MEMORY 1  
#define GLaDOS_IMPLEMENT_STRING 1  

#include "GLaDOS/GLaDOS.h"


// Fatal error in kernel: prints the error message and hangs.
void panic(const char *why,uint64_t number)
{
    println();
    println("======= GLaDOS kernel panic! =======");
    print(why);
    print(number);
    println();
    hang();
}

/// Halts forever.  (For example, after a panic.)
void hang(void)
{
    while(true) {}
}

/* Called by UEFI_CHECK macro */
void check_error(UINT64 error,const char *function,int line) {
  if (error!=0) {
    println("UEFI call error: ");
    print("UEFI error code =");
    print_hex(error);
    println();
    print(" returned from UEFI function=");
    println(function);
    println();
    print("  from source code line=");
    print(line);
    panic("UEFI Call error",error);
  }
}



/* The compiler seems to want a function with this name as soon
   as you mention a "noexcept" function like operator delete.
   
   Possibly related:
     https://docs.microsoft.com/en-us/cpp/c-runtime-library/cxxframehandler
*/
extern "C"
void __CxxFrameHandler3(uint64_t unknown_args) 
{
    panic("Compiler called __CxxFrameHandler3 (exception stuff?)",unknown_args);
}

extern "C"
void __std_terminate(uint64_t unknown_args) 
{
    panic("Compiler called __std_terminate (exception stuff?)",unknown_args);
}

/*
 clang automatically calls this function before declaring large local variables
  (if you don't add -mno-stack-arg-probe, only supported in recent clang builds).
 It's supposed to check to see if the stack is out of space.
*/
extern "C"
void __chkstk(void) 
{
    /* ignore for now */
}

/* clang automatically adds this if you declare a global with a destructor */
extern "C"
void atexit(void (*exit_function)(void))
{
    /* ignore for now, we don't even have shutdown yet */
}

/*---------- x86-64 utilities -------------*/
#pragma pack (1) //<- avoid padding this struct
struct amd64_descriptor {
    uint16_t sizeminus; // size in bytes, minus 1 because Intel
    uint64_t address; // location of descriptor data
};
struct amd64_idt_entry {
    uint16_t offset_0; // code address bits 0..15
    uint16_t segment; // a code segment selector in GDT or LDT
    uint8_t ist;       // bits 0..2 holds Interrupt Stack Table offset, rest of bits zero.
    uint8_t type:4; // gate type
    uint8_t z:1; // zero bit
    uint8_t dpl:2; // descriptor priv level
    uint8_t p:1; // present bit (1 for present)
    uint64_t offset_1; // code address bits 16..63, and some high zero bits
    uint16_t zero;     // reserved

    /// Fill in this idt entry's offset with this code address
    inline void set_address(uint64_t code_address)
    {
        offset_0=(uint16_t)code_address;
        offset_1=code_address>>16;
    }
};

void print(const amd64_idt_entry &e,int index) {
    print("interrupt "); print((uint64_t)index); print(" = ");
    uint64_t offset=(uint64_t)e.offset_0 + (e.offset_1<<16);
    print(offset); print(" segment "); print((uint64_t)e.segment); print("\n  ");
    if (e.ist!=0) { print(" ist="); print((int)e.ist); }
    print(" type="); print((int)e.type);
    print(" dpl="); print((int)e.dpl);
    print(" p="); print((int)e.p);
    if (e.zero!=0) { print(" reserved="); print((uint64_t)e.zero); }
    print("\n");
}

struct amd64_segment_descriptor {
    uint16_t limit_0; // segment limit
    uint16_t base_0; // base address bits 0..15
    uint8_t base_1:8; // more bits of base
    uint8_t type:4; 
    uint8_t s:1; // 0 for system, 1 for code or data
    uint8_t dpl:2; // descriptor priv level
    uint8_t p:1; // present bit (1 for present)
    uint8_t limit_1:4; // more bits of limit
    uint8_t avail:1; // 1 if available for system software
    uint8_t L:1; // 1 for long mode (64-bit)
    uint8_t DB:1; // 0 for 16 bit, 1 for 32 bit
    uint8_t G:1; // granularity
    uint8_t base_2; // last bits of base address
};

void print(const amd64_segment_descriptor &e,int index) {
    print("gdt "); print((uint64_t)index*sizeof(amd64_segment_descriptor)); print(" = ");
    uint64_t base=(uint64_t)e.base_0 + (e.base_1<<16) + (e.base_2<<24);
    print(" base="); print(base); 
    uint64_t limit=(uint64_t)e.limit_0 + (e.limit_1<<16);
    print(" limit="); print(limit);
    print("\n");
    print("   type="); print((int)e.type);
    print(" s="); print((int)e.s);
    print(" dpl="); print((int)e.dpl);
    print(" p="); print((int)e.p);
    print(" avl="); print((int)e.avail);
    print(" L="); print((int)e.L);
    print(" DB="); print((int)e.DB);
    print(" G="); print((int)e.G);
    print("\n");
}

#pragma pack ()

template <class entry>
entry *print_descriptors(const amd64_descriptor *desc) {
    print((int)sizeof(entry)); print(" bytes per entry\n");
    print(desc->sizeminus+1); print(" bytes at ");
    print(desc->address); print("\n");
    entry *entries=(entry *)desc->address;
    int n=(desc->sizeminus+1)/sizeof(entry);
    for (int i=0;i<n;i++)
    {
        print(entries[i],i);
        
        if (i%8==7) if (!pause()) break;
    }
    return entries;
}

// Store the CPU's current IDT/GDT into this descriptor.
extern "C" void sidt(amd64_descriptor *desc);
extern "C" void sgdt(amd64_descriptor *desc);
// Load the CPU's IDT/GDT from this descriptor.
extern "C" void lidt(const amd64_descriptor *desc);
extern "C" void lgdt(const amd64_descriptor *desc);


/// Explore the CPU-OS interface data structures
void print_idt(void)
{
    amd64_descriptor idt;
    sidt(&idt);
    print_descriptors<amd64_idt_entry>(&idt);
}

/// This is called by the CPU when an interrupt happens.
///   FIXME: figure out how to return from an interrupt.
void handle_interrupt(void)
{
    print("IT WORKS!\n");
    hang(); //<- it's tricky to return from an interrupt
}



/// Tell the CPU to run this code when this interrupt happens.
void hook_interrupt(int interrupt_number,uint64_t code_address)
{
    // Grab UEFI's existing IDT (kinda hacky)
    amd64_descriptor idt;
    sidt(&idt);
    amd64_idt_entry *idt_entries=(amd64_idt_entry *)idt.address;

    // Fill in the requested interrupt's handler:
    idt_entries[interrupt_number].set_address(code_address);

    // Update the CPU's IDT with our new mapping
    lidt(&idt);  //<- not actually needed, we changed the table in-place.
}

void test_idt(void)
{
    print("Hooking interrupt.\n");
    hook_interrupt(0xc0,(uint64_t)handle_interrupt);
    print("Trying to call it.\n");
    __asm__("int $0xc0");
    print("Back from interrupt?\n");
}

void print_gdt(void)
{
    amd64_descriptor gdt;
    sgdt(&gdt);
    print_descriptors<amd64_segment_descriptor>(&gdt);
}






