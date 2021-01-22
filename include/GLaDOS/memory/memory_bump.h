/**
 Simplest possible memory allocator for GLaDOS:
   a "bump" style memory allocator that never even frees.
   
  Dr. Orion Lawlor and the UAF CS 321 class, 2021-01 (Public Domain)
*/

/// This pointer is the start of unallocated memory
extern char *galloc_area; 

// Memory allocation.  Size is in bytes.
inline void *galloc(uint64_t size)
{
    void *buffer=galloc_area;
    galloc_area+=size;
    return buffer;
}

// Memory deallocation.  The ptr MUST have come from galloc.
inline void gfree(void *ptr)
{
    /* just ignore it for now! */
}

#if GLaDOS_IMPLEMENT_MEMORY /* definitions, in util.cpp */

///  It's just a random address we're claiming, 
///   hopefully it doesn't hit anything important in UEFI!
char *galloc_area=(char *)0x10000000; 


#endif


