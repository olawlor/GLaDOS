/*
  C++ classes for linked lists.
  
  Group Led and Designed Operating System (GLaDOS)
  A UEFI-based C++ operating system.
  
  Dr. Orion Lawlor and the UAF CS 321 class, 2021-01 (Public Domain)
*/
#ifndef __GLADOS_UTILITY_BYTEBUFFER_H
#define __GLADOS_UTILITY_BYTEBUFFER_H
#include <limits.h> // for INT_MAX and such.

/// Return the number of bytes in this nul-terminated C string.
///  (same as libc strlen, we just don't have libc here)
inline uint64_t strlen(const char *cstr) {
    const char *end=cstr;
    while (*end!=0) end++;
    return end-cstr;
}

/// Used when accessing raw memory
typedef unsigned char Byte;

/*
 Represents an area of memory, used for:
    - Memory allocation
    - File I/O
    - Network I/O
    - Bulk graphics operations like erase, copy, or fill
*/
class ByteBuffer {
public:
    // Create a ByteBuffer representing this start location and number of bytes.
    ByteBuffer(void *startPointer,uint64_t lengthInBytes)
    {
        start=(Byte *)startPointer;
        length=lengthInBytes;
    }
    
    // Create a ByteBuffer for this C string
    ByteBuffer(const char *str)
    {
        start=(Byte *)str;
        length=strlen(str);
    }
    
    // Empty ByteBuffer
    ByteBuffer()
    {
        start=0;
        length=0;
    }
    
    /// Extract a portion of this buffer starting this many bytes in.
    ByteBuffer splitAtByte(uint64_t startByte,uint64_t newLength)
    {
        // FIXME: bounds checks?
        return ByteBuffer(start+startByte,newLength);
    }

    /// Fill this buffer with this many copies of this object.
    ///  Returns the number of objects that actually fit.
    template <typename T>
    uint64_t Fill(const T &object, uint64_t numberOfObjects=LLONG_MAX){
        uint64_t Tlength = length - (length % (sizeof(T)));
        uint64_t reqLength=numberOfObjects*sizeof(T);
        uint64_t fillLength=std::min(Tlength,reqLength);        
    
        //fill the buffer with the objects
        uint64_t i;
        for(i = 0; i<fillLength; i+=sizeof(T)) {
            *(T *)(start + i) = object;
        }
        return i/sizeof(T);
    }
    
// FIXME: Add more utility functions here!
    
    // Support C++ iterator begin / end interface for bytes (Is limiting this to bytes a good idea?)
    Byte *begin() const { return start; }
    Byte *end() const { return start+length; }
    
    /// Get length, in bytes, of our buffer (can be zero)
    inline uint64_t getLength(void) const { return length; }
protected:
    Byte *start; // points to start of our buffer
    uint64_t length; // number of bytes in our buffer (can be zero)
};

/** A thin type layer around a ByteBuffer. */
template <typename T>
class Array : public ByteBuffer {
public:
    Array(T *ptr,uint64_t len_in_Ts) 
        :ByteBuffer(ptr,len_in_Ts*sizeof(T)) 
    {}
    Array(const ByteBuffer &src) :ByteBuffer(src) {}
    
    T *begin() const { return (T *)start; }
    T *end() const { return ((T *)start) + length/sizeof(T); }
};




#endif

