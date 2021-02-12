/*
  C++ classes for demand-created strings, a "string source".
  
  Group Led and Designed Operating System (GLaDOS)
  A UEFI-based C++ operating system.
  
  Dr. Orion Lawlor and the UAF CS 321 class, 2021-01 (Public Domain)
*/
#ifndef __GLADOS_UTILITY_STRINGSOURCE_H
#define __GLADOS_UTILITY_STRINGSOURCE_H


/**
  Represents a source of UTF-8 printable character data.
  Parent class, can be inherited from. 
  
  Normally doesn't actually store any data, just points to
  some existing copy, so it's cheap to create and manipulate.
  Long strings can be generated on the fly.
  
  Honestly should be abstract, but allowing construction from 
  bare buffers and "char *" makes string handling a lot cleaner for users.
  For example, a function taking a const StringSource &src
  can take a "char *" or a raw ByteBuffer as an argument.
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


/** Concatenate the data from two StringSource objects.
  This will output the data from s0 first, then the data from s1.
*/
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

/** This free operator+ lets you concatenate any two StringSources.
    auto fullName=baseName+".txt";
*/
inline ConcatStringSources operator+(const StringSource &s0,const StringSource &s1)
{
    return ConcatStringSources(s0,s1);
}


/**
 Lets you substitute a char for a ByteBuffer inside a string.
 This lets you, for example, replace '\n' (old) with "\r\n" (good).
*/
class TransformStringSource : public StringSource {
public:
    TransformStringSource(Byte old_,const ByteBuffer &good_,const StringSource &src_)
        :old(old_), good(good_), src(src_), srcIndex(0), offset(0) {}
    
    /*
     Scan through looking for the old char.
     Return src unmodified where possible,
     or substitute good for old.
    */
    bool get(ByteBuffer &buf,int index) const;
    
private:
    Byte old;
    ByteBuffer good;
    const StringSource &src;
    
    // Used during "get".  ASSUMES a single caller with clean index sweep (CAUTION!)
    mutable int srcIndex; // buffer index in src
    mutable ByteBuffer srcBuf; // scanning through this
    mutable uint64_t offset; // location in buf
};

/// Utilty function to build a transform string source.
///   Usage:   auto escaped=xform('/',"SLASH",filePathString);
inline TransformStringSource xform(Byte old,const ByteBuffer &good,const StringSource &src)
{
    return TransformStringSource(old,good,src);
}

#if GLaDOS_IMPLEMENT_STRING /* include function definitions */
/**
   Scan through src looking for the old char.
     Return src unmodified where possible,
     or substitute good for old.
  
  This is honestly quite confusing. 
*/
bool TransformStringSource::get(ByteBuffer &buf,int index) const
{
    if (offset>=srcBuf.getLength())
    { // Need to fetch the next source buffer
        if (!src.get(srcBuf,srcIndex)) 
        { // Now source string is done
            srcIndex=0; //<- get ready for next pass
            return false; 
        }
        srcIndex++; // we now have that in buf
        offset=0; // scan it from the beginning
    }
    
    // Find the end of the buffer, or the old char
    uint64_t start=offset;
    Byte *array=srcBuf.begin();
    while (true) {
        /* 
         srcBuf:
            start       old char     srcBuf end
                      A B            C
             -offset scans this way->
        */
        if (offset>=srcBuf.getLength())  // case C
        { // We walked off the end of this buffer, return last data
            buf=srcBuf.splitAtByte(start,offset-start);
            return offset-start>0;
        }
        if (array[offset]==old)
        { // We hit the old char
            if (start==offset) // case B
            { // We're ready to return the substitute buffer
                buf=good;
                offset++; // skip over the old data next time
                return true;
            }
            else { // case A
                // Return unmodified data up to old
                buf=srcBuf.splitAtByte(start,offset-start);
                return offset-start>0;
            }
        }
        offset++;
    }
}
#endif


/** Streams string data out of a file */
class FileDataStringSource : public StringSource {
public:
    FileDataStringSource(EFI_FILE_PROTOCOL* file_) { file=file_; }
    
    // Read file data (block by block)
    bool get(ByteBuffer &buf,int index) const;
    
private:
    EFI_FILE_PROTOCOL* file; // opened file (EFI)
    enum {BLOCK_SIZE=4096}; // I/O buffer size
    mutable Byte block[BLOCK_SIZE]; // I/O buffer
};

/// Return contents of a file as a StringSource
FileDataStringSource FileContents(const StringSource &filename);


/// Convert a StringSource to a UTF-16 buffer of CHAR16's, with nul terminator.
///  This is what most UEFI function calls need for strings.
///  Example: OutputString(CHAR16ify("foo"));
template <int MAXCHAR=1024>
class CHAR16ify {
public:
    CHAR16ify(const StringSource &str) {
        uint64_t out=0;
        int LASTCHAR=MAXCHAR-2; // leave space for '@' and nul terminator
        ByteBuffer buf; int strIndex=0;
        while (str.get(buf,strIndex++)) {
            for (char c:buf) {
                if (out<LASTCHAR)
                    wide[out++]=(CHAR16)c;
                else
                    break; // out of space in buffer
            }
        }
        if (out==LASTCHAR) wide[out++]='@'; //<- mark truncated output
        // Add nul terminator
        if (out<MAXCHAR) wide[out++]=0;
    }

    /// This converts us to a CHAR16 *, like UEFI wants.
    operator CHAR16 *() const { return (CHAR16 *)wide; }
private:
    CHAR16 wide[MAXCHAR];
};


/// Convert a StringSource to a vector of CHAR16.
///  This does dynamic allocation, but works with arbitrarily long strings.
vector<CHAR16> CHAR16_from_String(const StringSource &str);

#if GLaDOS_IMPLEMENT_STRING
vector<CHAR16> CHAR16_from_String(const StringSource &str)
{
    vector<CHAR16> wide;
    
    ByteBuffer buf; int index=0;
    while (str.get(buf,index++)) {
        for (char c:buf) {
            wide.push_back((CHAR16)c);
        }
    }
    
    wide.push_back((CHAR16)0); // add nul terminator char at end
    return wide;
}
#endif



#endif


