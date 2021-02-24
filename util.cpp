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
    while(true) { pause_CPU(); }
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
    uint8_t Z:1; // zero bit
    uint8_t dpl:2; // descriptor priv level
    uint8_t P:1; // present bit (1 for present)
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
    print(" P="); print((int)e.P);
    if (e.zero!=0) { print(" reserved="); print((uint64_t)e.zero); }
    print("\n");
}

// Segment descriptors live in the GDT:
struct amd64_segment_descriptor {
    uint16_t limit_0; // segment limit
    uint16_t base_0; // base address bits 0..15
    uint8_t base_1:8; // more bits of base
    uint8_t type:4; 
    uint8_t S:1; // 0 for system, 1 for code or data
    uint8_t dpl:2; // descriptor priv level
    uint8_t P:1; // present bit (1 for present)
    uint8_t limit_1:4; // more bits of limit
    uint8_t avail:1; // this space available for system software
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
    print(" S="); print((int)e.S);
    print(" dpl="); print((int)e.dpl);
    print(" P="); print((int)e.P);
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


/// Tell the CPU to run this code when this interrupt happens.
void hook_interrupt(int interrupt_number,uint64_t code_address)
{
    amd64_descriptor idt;
    sidt(&idt);
    amd64_idt_entry *table=(amd64_idt_entry *)idt.address;
    table[interrupt_number].set_address(code_address);
    lidt(&idt);
}

int CPU_error_leash=0;

/// Called by one of the interrupt handlers below, to handle a CPU error.
void handle_generic_CPU_error(const char *name,const char *desc)
{
    ++CPU_error_leash;
    if (CPU_error_leash>3) { // interrupt loop?
        cli(); // interrupts off
        hang(); // just stop trying to fix stuff...
    }
    
    print("-------- CPU Interrupt -------\n");
    print(name); print(" - "); print(desc);
    print("\nHalting.\n");
    hang();
}

void handle_DE(void)
{
    handle_generic_CPU_error("#DE","Divided by zero\n");
}
void handle_DB(void)
{
    handle_generic_CPU_error("#DB","Debug interrupt\n");
}
void handle_NMI(void)
{
    handle_generic_CPU_error("hw","Hardware non-maskable interrupt\n");
}
void handle_BP(void)
{
    handle_generic_CPU_error("#BP","Debug breakpoint / int 3\n");
}
void handle_OF(void)
{
    handle_generic_CPU_error("#OF","Overflow detected\n");
}
void handle_BR(void)
{
    handle_generic_CPU_error("#BR","Bound range exceeded\n");
}
void handle_UD(void)
{
    handle_generic_CPU_error("#UD","Undefined CPU opcode\n");
}
void handle_NM(void)
{
    handle_generic_CPU_error("#NM","Device not available\n");
}
void handle_DF(void)
{
    handle_generic_CPU_error("#DF","CPU error while servicing interrupt (bad IDT?)\n");
}
void handle_TF(void)
{
    handle_generic_CPU_error("#TF","Invalid TSS (what *IS* the TSS?!)\n");
}
void handle_NP(void)
{
    handle_generic_CPU_error("#NP","Segment not present\n");
}
void handle_SS(void)
{
    handle_generic_CPU_error("#SS","Bad stack segment load\n");
}
void handle_GP(void)
{
    handle_generic_CPU_error("#GP","General protection fault\n");
}
void handle_PF(void)
{
    handle_generic_CPU_error("#PF","Page table fault\n");
}
void handle_MF(void)
{
    handle_generic_CPU_error("#MF","Float exception on x87\n");
}
void handle_AC(void)
{
    handle_generic_CPU_error("#AC","Alignment check error (SIGBUS)\n");
}
void handle_MC(void)
{
    handle_generic_CPU_error("#MC","Machine check error\n");
}
void handle_XM(void)
{
    handle_generic_CPU_error("#XM","SSE float exception\n");
}
void handle_VE(void)
{
    handle_generic_CPU_error("#VE","Virtualization exception (VT-x extended page tables?)\n");
}
void handle_SX(void)
{
    handle_generic_CPU_error("#SX","Security exception in virtual machine\n");
}
void handle_intC0(void)
{
    print("0xc0> WOAH!!!!  IT WORKED!!!!\n");
    print("Got an int 0xc0.\n");
    hang();
}

/// Configure the Global Descriptor Table at OS boot
void setup_GDT(void)
{
    // Grab the existing GDT
    amd64_descriptor gdt;
    sgdt(&gdt);
    
    /*
     Standard UEFI puts code at segment descriptor 0x38.
     syscall STAR MSR expects a data segment at code segment +8.
     So we need to write a segment at 0x40.
    */
    // Existing data segment, set up by UEFI:
    amd64_segment_descriptor *seg30=(amd64_segment_descriptor *)(gdt.address+0x30);
    // Unused space after the code segment:
    amd64_segment_descriptor *seg40=(amd64_segment_descriptor *)(gdt.address+0x40);
    
    if (seg30->P==1 && seg40->P==0)
    { // copy existing data segment into unused entry
        *seg40 = *seg30;
    }
    else {
        print("GDT Warning: segments not where expected, expect failures.\n");
    }
    
    lgdt(&gdt);
    print(" gdt ");
}

/// Configure the Interrupt Descriptor Table at OS boot.
///  These print nice error messages when things go wrong.
void setup_IDT(void)
{
    hook_interrupt(0x0,(uint64_t)handle_DE);
    hook_interrupt(0x1,(uint64_t)handle_DB);
    hook_interrupt(0x2,(uint64_t)handle_NMI);
    hook_interrupt(0x3,(uint64_t)handle_BP); //<- hit by hardware?
    hook_interrupt(0x4,(uint64_t)handle_OF);
    hook_interrupt(0x5,(uint64_t)handle_BR);
    hook_interrupt(0x6,(uint64_t)handle_UD);
    hook_interrupt(0x7,(uint64_t)handle_NM);
    hook_interrupt(0x8,(uint64_t)handle_DF);
    hook_interrupt(0xA,(uint64_t)handle_TF);
    hook_interrupt(0xB,(uint64_t)handle_NP);
    hook_interrupt(0xC,(uint64_t)handle_SS);
    hook_interrupt(0xD,(uint64_t)handle_GP);
    hook_interrupt(0xE,(uint64_t)handle_PF);
    hook_interrupt(0x10,(uint64_t)handle_MF);
    hook_interrupt(0x11,(uint64_t)handle_AC);
    hook_interrupt(0x12,(uint64_t)handle_MC);
    hook_interrupt(0x13,(uint64_t)handle_XM);
    hook_interrupt(0x14,(uint64_t)handle_VE);
    hook_interrupt(0x1E,(uint64_t)handle_SX);
    print(" idt ");
}

void test_idt(void)
{
    hook_interrupt(0xc0,(uint64_t)handle_intC0);
    print("Call interrupt 0xc0.\n");
    __asm__("int $0xc0");
    print("Back to normal\n");
}

void print_gdt(void)
{
    amd64_descriptor gdt;
    sgdt(&gdt);
    print_descriptors<amd64_segment_descriptor>(&gdt);
}

void test_gdt(void)
{
    print("Grab the GDT\n");
    amd64_descriptor gdt;
    sgdt(&gdt);
    print("Write to the GDT:\n");
    
    // Main code segment
    amd64_segment_descriptor *seg=(amd64_segment_descriptor *)(gdt.address+0x38);
    
    seg->P=0; // make it not Present
    
    lgdt(&gdt);
    print("Done with GDT\n");
}


/******* Page Tables *********/

extern "C" void * read_CR3(void); //< in util_asm.s, reads cr3 register

void print_pagetables(void)
{
    print("Reading CR3\n");
    void *cr3=read_CR3();
    print((uint64_t)cr3);
    print("\n");
}

void test_pagetables(void)
{
    print("Trying to access a crazy address\n");
    int *ptr=(int *)0xbadc0def00; // "bad code foo"
    print(*ptr);
    print("Trying to write data there\n");
    *ptr=3;
    print(*ptr);
    print(" ... are we OK?\n");
}





