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
enum {PAGE_BITS=12}; // bits per page address
// These are the bits in a normal x86-64 linear address
struct pointer_bits_PML4 {
	uint64_t within_page:12; // byte offset inside the 4KB page
	uint64_t idx1:9; // index into PML1
	uint64_t idx2:9; // index into PML2
	uint64_t idx3:9; // index into PML3
	uint64_t idx4:9; // index into PML4
	uint64_t canonical:16; // must all be the same as high bit of idx4
};
// This union lets you extract the bits from a pointer
union pointer_to_bits_PML4 {
    void *ptr;
    pointer_bits_PML4 bits;
};

enum {PML_LEN=1<<9}; // pagemap length (=4KB/sizeof(pagemap_entry))
struct pagemap_entry {
	// 12 bits of flags:
	uint64_t present:1; // 1 if present, 0 if you want #PF on access
	uint64_t RW:1; // 1 if writeable, 0 for read only
	uint64_t US: 1; // 1 if accessible to user, 0 for system only
	uint64_t PWT:1; // writethrough cache
	uint64_t PCD:1; // page cache disable
	uint64_t A:1; // accessed (set by CPU)
	uint64_t D:1; // dirty (set by CPU on the last level)
	uint64_t PAT:1; // 0 for normal, 1 for special page (huge pages on PML2 & 3)
	uint64_t G:1; // 1 if "global", accessible everywhere
	uint64_t ign:3; // OS can use this, CPU ignores them

	uint64_t address:36; // physical address of next level, left shifted by 12 bits

	uint64_t reserved:15; // high bits mostly reserved
	uint64_t XD:1; // "execute disable": do not run code here if 1.
    
    // Get the next level entry
    pagemap_entry *next_level(void) const {
        if (!present) return 0;
        uint64_t addr=address<<PAGE_BITS;
        return (pagemap_entry *)addr;
    }
    
    // Print one page-map-level entry
    void print_entry(void)
    {
        print(" => ");
        print(address); 
           print("000"); //<- add back the 12 low bits
        print(": ");
        if (present) {
            if (RW) print("RW ");
            if (US) print("US ");
            if (PWT) print("PWT ");
            if (PCD) print("PCD ");
            if (A) print("A ");
            if (D) print("D ");
            if (PAT) print("PAT ");
            if (G) print("S ");
            if (XD) print("XD ");
        }
        else print("not present");
        print("\n");
    }
};

extern "C" pagemap_entry *read_CR3(void); //< in util_asm.s, reads cr3 register

// Pretty-print this index into our page-map level,
//   and return that entry.
pagemap_entry &print_index(pagemap_entry *pml,const char *level,int index) {
    print("  ");
    print(level);
    print(" = ");
    print((uint64_t)pml);
    print("["); print(index); print("]");
    pagemap_entry &entry=pml[index];
    entry.print_entry();
    return entry;
}

// Print the pagetable entries for this pointer:
void walk_pagetable(void *ptr)
{
    print("Walking pagetable for ");
    print((uint64_t)ptr);
    print("\n");
    pointer_bits_PML4 bits=*reinterpret_cast<pointer_bits_PML4 *>(&ptr);

    // Start at CR3, and walk down PML4, 3, 2, 1
    pagemap_entry *pml4=read_CR3();
    if (pml4==0) { print("  => not present (#PF)\n"); return; }
    pagemap_entry &pml4e=print_index(pml4,"PML4",bits.idx4);
    if (pml4e.PAT) { print("   => a 512GB(!) page\n"); return; }
    
    pagemap_entry *pml3=pml4e.next_level();
    if (pml3==0) { print("  => not present (#PF)\n"); return; }
    pagemap_entry &pml3e=print_index(pml3,"PML3",bits.idx3);
    if (pml3e.PAT) { print("   => a 1GB(!) page\n"); return; }
    
    pagemap_entry *pml2=pml3e.next_level();
    if (pml2==0) { print("  => not present (#PF)\n"); return; }
    pagemap_entry &pml2e=print_index(pml2,"PML2",bits.idx2);
    if (pml2e.PAT) { 
        print("   => a 2MB page, physical address "); 
        print((uint64_t)pml2e.next_level());
        print("\n");
        return; 
    }
    
    pagemap_entry *pml1=pml2e.next_level();
    if (pml1==0) { print("  => not present (#PF)\n"); return; }
    pagemap_entry &pml1e=print_index(pml1,"PML1",bits.idx1);
    print("   => a normal page, physical address ");
    print((uint64_t)pml1e.next_level());
    print("\n");
}

int random_global=0;
void print_pagetables(void)
{
    // Examine the pagetable entries for one address
    walk_pagetable(&random_global);
    
    print("\n");
}

void test_pagetables(void)
{
    print("Check pagetable for this crazy address:\n");
    int *ptr=(int *)0xbadc0def00; // "bad code foo"
    walk_pagetable(ptr);
    print("Trying to access a crazy address\n");
    print(*ptr);
    print("Trying to write data there\n");
    *ptr=3;
    print(*ptr);
    print(" ... are we OK?\n");
}





