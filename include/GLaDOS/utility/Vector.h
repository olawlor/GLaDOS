/*
  C++ classes for a dynamically allocated 1D array, 
  similar to std::vector.
  
  Group Led and Designed Operating System (GLaDOS)
  A UEFI-based C++ operating system.
  
  Dr. Orion Lawlor and the UAF CS 321 class, 2021-02 (Public Domain)
*/
#ifndef __GLADOS_UTILITY_VECTOR_H
#define __GLADOS_UTILITY_VECTOR_H


template <class T>
class vector {
public:
    // Handy typedefs, hat tip to libstdc++
    typedef T *pointer;
    typedef const T *const_pointer;
    typedef T *iterator;
    typedef const T *const_iterator;
    typedef T &reference;
    typedef const T &const_reference;
    typedef uint64_t size_type;
    
    /// Create an empty vector
    vector() { start=finish=end_of_storage=0; }
    
    /// Don't copy vectors
    vector(const vector &v) =delete;
    void operator=(const vector &v) =delete;
    
    /// But move construction is OK:
    vector(vector<T> && doomed) {
        start=doomed.start;
        finish=doomed.finish;
        end_of_storage=doomed.end_of_storage;
        
        doomed.start=doomed.finish=doomed.end_of_storage=0;
    }
    
    /// Deallocate memory on destruction
    ~vector() {
        if (start) delete[] start;
        start=finish=end_of_storage=(pointer)0xd;
    }
    
    /// Extract a const pointer to our first member
    operator const T *() { return start; }
    
    /// Return the number of elements currently in the vector.
    size_type size() const { return finish-start; }
    /// Return the number of elements that can be stored without resizing.
    size_type capacity() const { return end_of_storage-start; }
    /// Return true if we contain no elements
    bool empty() const { return begin()==end(); }
    
    /// Append this element to the end of this vector.
    void push_back(const T &v) 
    {
        if (finish>=end_of_storage)
            reallocate();
        *finish++ = v;
    }
    
    /// Perform bounds checking on this array index, and panic if it's out of bounds.
    inline void bounds_check(size_type index) const {
        if (index<0 || index>=size())
            panic("vector bounds check fails on index ",index);
    }
    
    reference operator[](size_type index) { 
#if GLADOS_BOUNDSCHECK
        bounds_check(index);
#endif
        return *(start+index);
    }
    const_reference operator[](size_type index) const { 
#if GLADOS_BOUNDSCHECK
        bounds_check(index);
#endif
        return *(start+index);
    }
    
    // Allow C++ range-based for by providing begin and end iterators
    iterator begin() { return start; }
    iterator end() { return finish; }
    const_iterator begin() const { return start; }
    const_iterator end() const { return finish; }

    /// Allocate enough memory to store more elements (at least to size()+1)
    void reallocate(void)
    {
        size_t new_bytes=8; // size in bytes
        while ((size()+1)*sizeof(T)>new_bytes) new_bytes*=2;
        size_t new_elements=new_bytes/sizeof(T);
        pointer nu=new T[new_elements];
        
        // Copy old to new elements
        // FIXME: figure out fancy placement new here, to avoid copying T's
        // FIXME: make this exception safe
        size_type limit=size();
        for (size_type index=0;index<limit;index++)
            nu[index]=start[index];
        
        pointer old=start;
        start=nu;
        finish=&nu[limit];
        end_of_storage=&nu[new_elements];
        delete[] old;
    }

private:
    pointer start; // galloc-allocated memory
    pointer finish; // valid (constructed) items
    pointer end_of_storage; // end of allocated memory
};



#endif

