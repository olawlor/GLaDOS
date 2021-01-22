/*
  GLaDOS generic memory allocation header.
  
  Dr. Orion Lawlor and the UAF CS 321 class, 2021-01 (Public Domain)
*/
#ifndef __GLADOS_MEMORY_H
#define __GLADOS_MEMORY_H

// These implementation files bring in galloc and gfree.
//   You only need one of them, comment out the unused ones.
//   (and feel free to add a new one with the same interface!)
//#include "memory_bump.h"
#include "memory_region.h"


#if GLaDOS_IMPLEMENT_MEMORY
// This adapts galloc/gfree over to C++ new and delete
//  They really should be inline, but C++ forbids this.
void* operator new  ( uint64_t count )
{ 
    return galloc(count); 
}
void* operator new[]  ( uint64_t count )
{ 
    return galloc(count); 
}

void operator delete (void *ptr) noexcept
{
    gfree(ptr);
}
void operator delete[] (void *ptr) noexcept
{
    gfree(ptr);
}
#endif

#endif

