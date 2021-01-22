/**
 Fancy memory allocator for GLaDOS
 A region-based allocator described here:
 https://docs.google.com/document/d/e/2PACX-1vTLGRjML5SZEgw3YVYyWQcAMLDv5TN-jFz-lJrJpOZU3Jisi6ohAJKCABJk6mazOsCv_s3LRLvyKzoW/pub
   
  Dr. Orion Lawlor and the UAF CS 321 class, 2021-01 (Public Domain)
*/

// This is the memory address where the region allocations start.
//  The high bits of this pointer are arbitrary, but
//   hopefully it doesn't hit anything important in UEFI!
const uint64_t REGION_BASE=0x10000000; 


// This datatype stores a block's region number, 
//  from 0 (8 bytes in size) to 63 (invalid / overflow)
typedef unsigned int region_t;

// We have a separate region for each power of two size.
//    High region numbers will always stay zero. 
enum {NUM_REGIONS=64 };

// REGION_SHIFT is the number of bits in an allocated pointer
//  before the region number starts.  
// 2^REGION_SHIFT is the size, in bytes, of all the space available for a region
enum { REGION_SHIFT=20 };


// Extract the region number from a pointer
inline region_t region_for_pointer(void *ptr)
{
	uint64_t p=(uint64_t)ptr;
	return (p-REGION_BASE)>>REGION_SHIFT;
}

// Return the pointer for the first block in this region
inline void *pointer_for_region(region_t r)
{
	uint64_t p=(((uint64_t)r)<<REGION_SHIFT)+REGION_BASE;
	return (void *)p;
}

// Compute a region number for a size, in bytes
inline region_t region_for_size(uint64_t size)
{
	if (size<=8) return 0; //<- clz is undefined around 0

	// Rationale: round up to multiple of 4,
	//  subtract 1 to handle if it's already a power of 2,
	//  count leading zeros (clz)
	//  subtract from 63 (using xor)
	else return (__builtin_clzll((size+3)/4-1)^63); 
}

// Compute this region's size, in bytes
inline uint64_t size_for_region(region_t r)
{
	return 8ull<<r;
}

// Memory blocks not in use are stored in this linked list.
struct region_freelist {
	region_freelist *next; // points to next entry in list, or 0 if end.
};

/// Each *thread* should store a separate free list for each region.
extern region_freelist *freelist[NUM_REGIONS];

/// This is called when we need to add space to the freelist
void *galloc_slowpath(uint64_t size);

/// Inlined (fast path) memory allocation.  This works like malloc/calloc
inline void *galloc(uint64_t size)
{
	region_t r=region_for_size(size);
	region_freelist *buffer=freelist[r]; // check the freelist head
	if (buffer) {
		freelist[r]=buffer->next; // remove us from the list
		buffer->next=0; // whole buffer should be zeroed before user gets it
		return buffer;
	}
	else 
		return galloc_slowpath(size); //<- fills freelist, handles errors
}

// Memory deallocation.  The ptr MUST have come from galloc.
inline void gfree(void *ptr)
{
	region_t r=region_for_pointer(ptr);
	region_freelist *buffer=(region_freelist *)ptr;

#if 1 //<- fixme: add a GLaDOS_PARANOID config option (and a config.h!)
	// Scrub all user data from this buffer (for security)
	//  Entry i=0 becomes buffer->next and is overwritten below.
	uint64_t buffersize=(1ULL<<r); // size in 8-byte uint64_t's
	for (uint64_t i=1;i<buffersize;i++)
		((uint64_t *)buffer)[i]=0ul;
#endif

	// Link the buffer into our freelist
	buffer->next=freelist[r];
	freelist[r]=buffer;
}




#if GLaDOS_IMPLEMENT_MEMORY /* definitions, in util.cpp */


/// Each *thread* should store a separate free list for each region.
region_freelist *freelist[NUM_REGIONS]={0};



/// This is called when we need to add space to the freelist
void *galloc_slowpath(uint64_t size)
{
	region_t r=region_for_size(size);
	if (r<0||r>REGION_SHIFT) {
		// Allocation too big--fall back to page table here?
		panic("Allocation too big for galloc:",size);
	}
	
	// Allocate some memory
	//  FIXME: check if we've already allocated too much and are hitting the next region
	//  FIXME: direct page table alloc here?  Multicore locking / buffer stealing here?
	void *base=pointer_for_region(r);
	uint64_t buffersize=size_for_region(r); // size of buffers to allocate
	enum {n_bytes=1024*1024}; // allocate this many bytes per mmap call
	
	// Split the new space into buffers, and link into the free list.
	//  Invariant: buffers in the free list are all zeroed
	uint64_t nbuffers=n_bytes/buffersize;
	//printf("galloc: Initializing %d buffers starting at %p for region %d\n",
	//	(int)nbuffers,base,(int)r);
	print("galloc: Initializing buffers for region ");
	print((int)r);
	print(" at pointer ");
	print((uint64_t)base);
	println();
	
	for (int b=nbuffers-1;b>=0;b--)
	{
		char *start=((char *)base)+b*buffersize;
		region_freelist *buffer=(region_freelist *)start;
		if (b==0) {
		    print("   Finished, freelist=");
		    print((uint64_t)freelist[r]);
		    println();
		    
		    return buffer; //<- return the last buffer to the user
		}
		// link the other buffers into the freelist
		buffer->next=freelist[r];
		freelist[r]=buffer;
	}
	return 0; //<- never reached, we return at b==0 (but gcc whines)
}

#endif


