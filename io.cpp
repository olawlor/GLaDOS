/*
  Input/Output handling for GLaDOS
  
  Dr. Orion Lawlor and the UAF CS 321 class, 2021-01 (Public Domain)
*/
#include "GLaDOS/GLaDOS.h"


/* Print an ordinary ASCII string */
void print(const char *str) {
  // Widen string to UTF-16 used by UEFI
  const int n=128;
  CHAR16 widestr[n];
  int out=0;
  while (*str!=0 && out<n-2) { 
    if (*str=='\n') widestr[out++]=(CHAR16)'\r'; 
    widestr[out++]=(CHAR16)*str++; 
  }
  widestr[out++]=0; // add nul char at end
  
  // Call UEFI to actually print the string
  ST->ConOut->OutputString(ST->ConOut,widestr);
}

/* Print a string, plus a newline */
void println(const char *data) {
  if (data) print(data);
  print("\n");
}

/* Print an unsigned long as this many hex digits */
void print_hex(uint64_t value,long digits,char separator) {
  char buf[20];
  const char *chars="0123456789ABCDEF";
  int out=0;
  buf[out++]='0'; // add leading hex
  buf[out++]='x';
  for (int digit=digits-1;digit>=0;digit--) 
    buf[out++]=chars[(value>>(4*digit))&0xF];
  if (separator) buf[out++]=separator;
  buf[out++]=0; // add nul
  print(buf);
}

/* Print a signed (+/-) integer in this base, from 2 to 16 */
void print_long_internal(int64_t value,long base=10,long min_length=1,char separator=' ') {
  const char *chars="0123456789ABCDEF";
  char buf[70];
  int out=69;
  buf[out--]=0; // ending nul
  if (separator) buf[out--]=separator;
  bool negative=false;
  if (value<0) { value=-value; negative=true; }
  
  // Peel out the digits *backwards*, starting from the little end.
  int start=out;
  while (value!=0 && out>0) {
    buf[out--]=chars[value%base];
    value=value/base;
  }
  
  // Pad to the requested length
  while ((start-out)<min_length && out>0) buf[out--]='0';
  
  // Add 0x or 0b, for hex or binary
  if (out>1) {
    if (base==16) { buf[out--]='x'; buf[out--]='0'; }
    if (base==2)  { buf[out--]='b'; buf[out--]='0'; }
  }
  
  // Add minus sign
  if (negative && out>0) buf[out--]='-';
  print(&buf[out+1]);
}

void print(int64_t value) {
  print_long_internal(value,10);
  print("(");
  print_long_internal(value,16,1,0);
  print(") ");
}
void print(int value) {
  print((int64_t)value);
}
void print(uint64_t value) {
  print_long_internal(value,16,1,0);
}


// Scan codes for various key presses
enum {
  keycode_up=-1,
  keycode_down=-2,
  keycode_right=-3,
  keycode_left=-4,
  keycode_Fkey=-10,
  keycode_esc=-23
};

/*
 Return the read-in char, as Unicode (if possible).
*/
int read_char(void) {
  EFI_STATUS status;
  EFI_INPUT_KEY k;
  do {
    status = ST->ConIn->ReadKeyStroke(ST->ConIn, &k);
  } while (status==EFI_NOT_READY);
  if (k.UnicodeChar) {
    if (k.UnicodeChar==0xD) return '\n'; // 0xA
    else return k.UnicodeChar;
  }
  else return -k.ScanCode;
}

/* Return true if we should keep scrolling, false if the user hits escape */
bool pause(void) {
  println("Press ESC to stop, any other key to continue..."); 
  return read_char()!=-23;
}

void clear_screen(void) {
  ST->ConOut->ClearScreen(ST->ConOut);
}



void handle_commands(void)
{
  println();
  println("Enter crazy one-char commands here:");
 
  while(1) {
    print("> ");
    int cmd=read_char();
    print(cmd); 
    println();
    
    if (cmd==keycode_esc) { // escape key: just clears screen
      clear_screen();
    }
    else if (cmd=='i') { // stop handling interrupts
      println("Interrupts going off...");
      cli();
    }
    else if (cmd=='I') { // resume handling interrupts
      sti();
      println("Interrupts back on");
    }
    
    else if (cmd=='v') { // VGA memory
      // The old VGA addresses don't seem to work in EFI:
      char *poke=(char *)0xA0000;
      poke[0]='?';
      poke=(char *)0xB8000;
      poke[0]='!';
    }
    
    else if (cmd=='p') { // play with pointers
      // You can read and write to almost anywhere in low memory
      char *ptr=(char *)0xC0DE00000L; 
      for (int i=0;i<10;i++) {
         *ptr=0x12;
         print((uint64_t)ptr); print(" = "); print(*ptr); println();
         ptr++;
      }
    }
    else if (cmd=='n') { // try out operator new
        int *p=new int;
        print("Operator new returns pointer: ");
        print((uint64_t)p);
        *p=3;
        print(" and writing a 3 reads back ");
        print(*p);
        println();
    }
    else if (cmd=='f') { // read a file 
      // See: https://stackoverflow.com/questions/32324109/can-i-write-on-my-local-filesystem-using-efi
      
      println("Getting file services:");
      EFI_GUID guid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
      
      EFI_HANDLE* handles = 0;   
      UINTN handleCount = 0;
      UEFI_CHECK(ST->BootServices->LocateHandleBuffer(
        ByProtocol, &guid, NULL, &handleCount, &handles));

      print(handleCount);
      
      EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* fs = 0;
      UEFI_CHECK(ST->BootServices->HandleProtocol(
        handles[0],&guid,(void **)&fs));
      
      
      println("Opening root volume:");
      EFI_FILE_PROTOCOL* root = 0;
      UEFI_CHECK(fs->OpenVolume(fs, &root));
      
      EFI_FILE_PROTOCOL* file = 0;
      UEFI_CHECK(root->Open(root,&file,
        (CHAR16 *)L"APPS\\DATA.DAT", EFI_FILE_MODE_READ, 
        EFI_FILE_READ_ONLY | EFI_FILE_HIDDEN | EFI_FILE_SYSTEM));
      
      char buf[1000];
      UINTN size=sizeof(buf);
      UEFI_CHECK(file->Read(file,&size,buf));
      
      println("Read data:");
      println(buf);
      
      UEFI_CHECK(ST->BootServices->FreePool(handles));
    }
    
    else if (cmd=='m') { // dump memory map
      println("Fetching memory map");
      enum {n=128};
      EFI_MEMORY_DESCRIPTOR md[n];
      UINTN md_size=sizeof(md);
      UINTN key=0, ds=0;
      UINT32 dv=0;
      UEFI_CHECK(ST->BootServices->GetMemoryMap(&md_size,md,&key,&ds,&dv));
      
      print("Returned bytes per memory descriptor="); print(ds); 
      print(" vs my "); print(sizeof(md[0])); 
      println();
      
      if (ds) {
        print("Returned md_size="); print(md_size/ds); println();
        int counter=0;
        for (UINTN index=0;index<md_size;index+=ds) 
        {
          EFI_MEMORY_DESCRIPTOR *m=(EFI_MEMORY_DESCRIPTOR *)(index+(char *)md);
          
          uint64_t start=m->PhysicalStart;
          uint64_t pages=m->NumberOfPages;
          uint64_t end=start + 4096*pages;
          uint64_t attr=m->Attribute;
          print("Phys="); print_hex(start); print("to "); print_hex(end); 
          print("attr "); print_hex(attr,8); 
          const static char *mem_types[]={ // from Intel Table 5-5
            "0-reserved","1-loadercode","2-loaderdata","3-bootcode","4-bootdata",
            "5-runcode","6-rundata","7-free","8-error","9-ACPI","10-ACPINVS",
            "11-MMIO","12-MMIOport","13-pal","14-FUTURE"};
          print(mem_types[m->Type]); 
          println();
          if (counter++%8==7) if (!pause()) break;
        }
      }
      
    }
    else {
      println("Unknown command.");
    }
    
  } // keep interpreting commands forever
}

