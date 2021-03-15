/*
  Group Led and Designed Operating System (GLaDOS)
  
  Allocates physical memory, and provides functions
  to deal with the hardware page table.
  
  Dr. Orion Lawlor and the UAF CS 321 class, 2021-03 (Public Domain)
*/
#ifndef __GLADOS_PAGETABLE_H
#define __GLADOS_PAGETABLE_H

/// A physical memory address points to real hardware memory, inside the page table.
typedef uint64_t PhysicalAddress;

/// A virtual memory address is a normal pointer, as used by the kernel or programs.
typedef uint64_t VirtualAddress;

/// Page size, in bytes
enum {PageSize=4096}; 

/// Page allocator: allocates one 4KB page of physical memory.
///  If no physical memory is free, this panics.
PhysicalAddress AllocatePage(void);

/// Deallocate this 4KB page of physical memory.
void DeallocatePage(PhysicalAddress base);


/// Abstract Page access permissions
/// PagePermissions include: read, write, execute, and userspace access
typedef enum { None=0, Readable=1, Writable=2, Executable=4, UserAccess=8 } PagePermissions;

/// A set includes a number of page permissions combined:
class SetOfPagePermissions {
public:
	SetOfPagePermissions(PagePermissions perm=None) :flags(perm) {}
	
	/// You can combine sets with the bar operator:
	SetOfPagePermissions operator|(PagePermissions perm) const {
		return SetOfPagePermissions(perm|flags);
	}
	
	/// You can extract a value from the set with the ampersand operator:
	bool operator&(PagePermissions perm) const {
		return (flags&perm)==perm;
	}
	
	/// This constructor is used when building from a set of permissions:
	SetOfPagePermissions(int newflags) :flags(newflags) {}
	
	void print(void) {
		if (flags & Readable) ::print("Readable ");
		if (flags & Writable) ::print("Writable ");
		if (flags & Executable) ::print("Executable ");
	}
private:
	int flags;
};




/// Page Table: a hardware-coupled data structure used to 
///  translate virtual addresses into physical addresses.
class PageTable {
public:
    PageTable() {
        base=0; ///<- lazy allocation on use
    }
    
    /// Add a page with these permissions to this pagetable:
    void add(PhysicalAddress page,VirtualAddress map,
        SetOfPagePermissions perm);
    
    /// Swap in this pagetable to be the current one used by the hardware:
    void activate(void);
    
private:
    PhysicalAddress base; ///<- hardware-specific start of storage.
};




#endif

