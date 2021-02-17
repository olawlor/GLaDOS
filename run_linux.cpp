/*
Load Linux ELF executables and run them.

Dr. Orion Lawlor and CS 321 class, 2021-02 (Public Domain)
*/
#include "GLaDOS/GLaDOS.h"
#include "elf.h"

// From asm_util.s:
typedef long (*function_t)(void);
extern "C" uint64_t start_function_with_stack(function_t f,uint64_t *stack);
extern "C" void return_to_main_stack(void);

extern "C" uint64_t syscall_setup(); 
extern "C" void syscall_finish(); 

// Syscall numbers from https://chromium.googlesource.com/chromiumos/docs/+/master/constants/syscalls.md
enum {
    syscallWrite=1,
    syscallOpen=2,
    syscallArch_prctl=158,
    syscallExit=60
};

extern "C" uint64_t handle_syscall(uint64_t syscallNumber,uint64_t *args)
{
    print("  syscall ");
    print((int)syscallNumber);
    if (syscallNumber==syscallWrite) {
        int fd=args[0];
        void *ptr=(void *)args[1];
        uint64_t len=args[2];
        print("write(");
        print(fd);
        println(")");
        if (fd==1) // stdout
            print(ByteBuffer(ptr,len));
        else
            panic("Unknown fd",fd);
    }
    else if (syscallNumber==syscallOpen) {
        const char *pathname=(const char *)args[0];
        uint64_t flags=args[1];
        uint64_t mode=args[2];
        print("open(");
        print(pathname);
        print(",");
        print(flags);
        print(",");
        print(mode);
        println(")");
        return -1; // return error (no open yet!)
    }
    else if (syscallNumber==syscallExit) {
        print("exit(");
        int exitcode=args[0];
        print(exitcode);
        println(")");
        // FIXME: switch back to kernel main stack
        return_to_main_stack();
        panic("tried to return to main stack, but didn't!");
    }
    else panic("Unknown syscall",syscallNumber);
    return 0;
}

// Put this file's data into memory at this address.
//  (FIXME: into program memory, not kernel memory!)
void map_file_to_memory(FileDataStringSource &exe,
    uint64_t file_offset, uint64_t size, uint64_t address)
{
    const uint64_t BLOCK=FileDataStringSource::BLOCK_SIZE;
    uint64_t index=file_offset/BLOCK;
    uint64_t round_down=file_offset%BLOCK;
    address-=round_down;
    uint64_t end_index=(round_down+size+BLOCK-1)/BLOCK;
    
    print("Filling program address ");
    print(address);
    print(" from file block ");
    print(index);
    println();
    
    // Copy the file data out to memory
    ByteBuffer buf;
    Byte *out=(Byte *)address;
    while (exe.get(buf,index++)) {
        for (Byte b:buf) *out++=b;
        if (index>=end_index) break;
    }
    
}

// Set up a new stack for a new Linux program.
//  Returns the new stack pointer the program can use.
//  Initialized according to the Linux SysV ABI here:
//    https://uclibc.org/docs/psABI-x86_64.pdf  
//  (See Section 3.4, Figure 3.9)
uint64_t *setup_stack(const char *program_name,uint64_t *start,uint64_t STACKSIZE)
{
    // Stack grows to lower addresses, so start at end of buffer.
    uint64_t *rsp=&start[STACKSIZE];
    
    *(--rsp)=0; // "push" null auxvector entry
    // auxvector entries go here: see https://github.com/torvalds/linux/blob/master/include/uapi/linux/auxvec.h
    //  Both glibc and diet libc really seem to need these or they die at startup...
    
    *(--rsp)=0; // "push" null after environment variables
    // environment variables go here
    *(--rsp)=0; // last argument 
    *(--rsp)=(uint64_t)program_name; // program arguments go here
    *(--rsp)=1; // number of arguments, including program name itself
    return rsp; //<- from the ABI, QWORD[rsp] == argc
}


// Load and execute a Linux program from this file:
int run_linux(const char *program_name)
{
  // Program p("APPS/PROG"); // <- aspirational interface
  
  // Load a program's ELF header
  FileDataStringSource exeELF=FileContents(program_name);
  ByteBuffer elfHeader;
  if (!exeELF.get(elfHeader,0)) {
    print("Can't read ELF file.\n");
    return -101;
  }
  const Byte *elfHeaderData=elfHeader.begin();
  const Elf64_Ehdr *elf=(const Elf64_Ehdr *)elfHeaderData;
  if (elf->e_machine!=EM_X86_64) {
    print("Wrong arch!\n");
    return -102;
  }
  
  // Load each of the program's segments
  //  SUBTLE: ELF header will go away as we read, so open file again.
  FileDataStringSource exe=FileContents(program_name);
  
  // Map in each of the file's segments
  //   FIXME: sanity check these before mapping in
  for (int p=0;p<elf->e_phnum;p++) {
        const Byte *pstart=elfHeaderData + elf->e_phoff + p*elf->e_phentsize;
        const Elf64_Phdr *ph=(const Elf64_Phdr *)pstart;
        if (ph->p_type==PT_LOAD) // <- we only care about loadable segments
        {
        /*  // FIXME: respect RWX flags for file's pieces
            bool R=(ph->p_flags&PF_R);
            bool W=(ph->p_flags&PF_W);
            bool X=(ph->p_flags&PF_X);
        */
            
            map_file_to_memory(exe,ph->p_offset,ph->p_memsz,ph->p_vaddr);
        }
  }
  
  // Set up syscalls  
  print("syscalls\n");
  syscall_setup();
  
  // Run the program
  function_t f=(function_t)elf->e_entry;
  
  print("Allocating stack\n");
  enum {STACKSIZE=32*1024};
  uint64_t *stack=new uint64_t[STACKSIZE];
  uint64_t *new_rsp=setup_stack(program_name,stack,STACKSIZE);
  print("Running linux program {\n");
  
  int ret=start_function_with_stack(f,new_rsp);
  
  print("}\n");
  
  print((int)ret);
  print(" was the return value.\n");
  println();

  syscall_finish();
  
  delete[] stack;
  
  return 0; // it worked!
}


