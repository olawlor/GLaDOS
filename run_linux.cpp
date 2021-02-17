/*
Load Linux ELF executables and run them.

Dr. Orion Lawlor and CS 321 class, 2021-02 (Public Domain)
*/
#include "GLaDOS/GLaDOS.h"

// From asm_util.s:
typedef long (*function_t)(void);
extern "C" uint64_t start_function_with_stack(function_t f,uint64_t *stack);
extern "C" void return_to_main_stack(void);

extern "C" uint64_t syscall_setup(); 
extern "C" void syscall_finish(); 


extern "C" uint64_t handle_syscall(uint64_t syscallNumber,uint64_t *args)
{
    print("  syscall ");
    print((int)syscallNumber);
    if (syscallNumber==1) {
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
    else if (syscallNumber==60) {
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



void run_linux(const char *program_name)
{
  // Program p("APPS/PROG"); // <- aspirational interface
  
  // Set up syscalls  
  print("syscalls\n");
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
   
  function_t f=(function_t)0x401000;
  
  print("Allocating stack\n");
  enum {STACKSIZE=32*1024};
  uint64_t *stack=new uint64_t[STACKSIZE];
  
  print("Running demo {\n");
  
  int ret=start_function_with_stack(f,&stack[STACKSIZE-1]);
  
  print("}\n");
  print((int)ret);
  print(" was the return value.\n");
  println();

  syscall_finish();
}


