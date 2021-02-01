/*
  C++ classes for linked lists.
  
  Group Led and Designed Operating System (GLaDOS)
  A UEFI-based C++ operating system.
  
  Dr. Orion Lawlor and the UAF CS 321 class, 2021-01 (Public Domain)
*/
#ifndef __GLADOS_UTILITY_STRINGSOURCE_H
#define __GLADOS_UTILITY_STRINGSOURCE_H


/* Represents a source of UTF-8 printable character data.
  Parent class, can be inherited from. 
  
  Honestly should be abstract, but allowing construction from 
  bare buffers and "char *" makes string handling a lot cleaner for users.
*/
class StringSource {
public:
    /// Create a source to read from this ByteBuffer
    StringSource(const ByteBuffer &buf=ByteBuffer())
        :single(buf) {}
    
    /// Create a source to read from this C string
    StringSource(const char *c_string)
        :single(c_string) {}
    
    /**
     Read another block of character data from this string.  
        Returns a valid buffer and true if there is data at this buffer index.
        Returns false if there is not data at this buffer index.
     
     Idiomatic call:
            ByteBuffer buf; int index=0;
            while (str.get(buf,index++)) {
                ... do something with buf data ...
            }
    */
    virtual bool get(ByteBuffer &buf,int index) const
    {
        // Assume we're a single buffer, unless a child overrides this method.
        if (index==0) {
            buf=single;
            return true;
        }
        else { // no more string
            return false;
        }
    }

protected:
    // *If* we only represent a single buffer, this is it:
    ByteBuffer single;
};


/* Concatenate the data from two StringSource objects */
class ConcatStringSources final : public StringSource {
public:
    ConcatStringSources(const StringSource &s0_,const StringSource &s1_)
        :s0(s0_), s1(s1_), endIndex0(INT_MAX)
    {}
    
    inline bool get(ByteBuffer &buf,int index) const 
    {
        if (index<endIndex0) {
            // Try string s0 first:
            if (s0.get(buf,index)) {
                return true;
            }
            else 
            { // That was too far for s0, so back off and try s1
                endIndex0=index;
            }
        }
        // else s0 is done, switch to s1
        return s1.get(buf,index-endIndex0);
    }

private:
    const StringSource &s0;
    const StringSource &s1;
    /// "mutable" is an end run around get being const.
    ///  This is kinda hacky, possibly a mistake.
    mutable int endIndex0;
};

/* This free operator+ lets you combine two StringSources */
inline ConcatStringSources operator+(const StringSource &s0,const StringSource &s1)
{
    return ConcatStringSources(s0,s1);
}


/* This one is kinda a mess, but lets you substitute a char for a ByteBuffer.
 This lets you, for example, replace '\n' (old) with "\r\n" (good).
*/
class TransformStringSource : public StringSource {
public:
    TransformStringSource(Byte old_,const ByteBuffer &good_,const StringSource &src_)
        :old(old_), good(good_), src(src_), srcIndex(0), offset(0) {}
    
    /*
     Scan through looking for the old char.
     FIXME: this is horrifically confusing, and misuses "mutable" badly.
     The key problem seems to be using our own index variables.
    */
    bool get(ByteBuffer &buf,int index) const 
    {
        if (offset>=srcBuf.getLength())
        {
            // Need to fetch the next source buffer
            if (!src.get(srcBuf,srcIndex)) return false; // source is done
            srcIndex++; // we now have that in buf
            offset=0; // scan it from the beginning
        }
        
        // Find the end of the buffer, or the old char
        uint64_t start=offset;
        Byte *array=srcBuf.begin();
        while (true) {
            if (offset>=srcBuf.getLength()) 
            { // We walked off the end of this buffer, return old data
                buf=srcBuf.splitAtByte(start,offset-start);
                return true;
            }
            if (array[offset]==old)
            { // We hit the old char
                if (start==offset) 
                { // We're ready to return the substitute buffer
                    buf=good;
                    offset++; // skip over the old data next time
                    return true;
                }
                else {
                    // Return unmodified data up to old
                    buf=srcBuf.splitAtByte(start,offset-start);
                    return true;
                }
            }
            offset++;
        }
    }
    
private:
    Byte old;
    const ByteBuffer &good;
    const StringSource &src;
    
    // Used during "get".  ASSUMES a single caller with clean index sweep (CAUTION!)
    mutable int srcIndex; // buffer index in src
    mutable ByteBuffer srcBuf; // scanning through this
    mutable uint64_t offset; // location in buf
};

// Utilty function to build a transform string source
inline TransformStringSource xform(Byte old,const ByteBuffer &good,const StringSource &src)
{
    return TransformStringSource(old,good,src);
}



#endif


