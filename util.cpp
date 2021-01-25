/*
  Utility functions for GLaDOS
  
  Dr. Orion Lawlor and the UAF CS 321 class, 2021-01 (Public Domain)
*/


#define GLaDOS_IMPLEMENT_MEMORY 1  /* make headers actually include function calls */

#include "GLaDOS/GLaDOS.h"


// Fatal error in kernel: prints the error message and hangs.
void panic(const char *why,uint64_t number)
{
    println();
    println("======= GLaDOS kernel panic! =======");
    print(why);
    print(number);
    println();
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
