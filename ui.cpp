/* 
  Receive user interface events like the keyboard and mouse.
  
  Uses ancient DOS-era PS/2 mouse input, because that's all that seems to work.
  
  UEFI has two mouse protocols (Simple Pointer and Absolute Pointer).
  For some reason, neither of them(!) work in either QEMU or VirtualBox.
    https://forum.osdev.org/viewtopic.php?f=1&t=37782&start=0
    
  Dr. Orion Lawlor and the UAF CS 321 class, 2021-03 (Public Domain)
*/
#include "GLaDOS/GLaDOS.h"
#include "GLaDOS/gui/event.h"


// See: https://wiki.osdev.org/Mouse_Input
// PS/2 support code from https://forum.osdev.org/viewtopic.php?t=10247
//   by SANiK, "Free PS/2 mouse code", 2005
//   "there are no royalties and crap like that."

int mouse_busywait_time=10000;
// Wait for bit 1 of port 64 to go low
void mouse_wait_write() {
  for (volatile int i=mouse_busywait_time;i>0;i--)
      if((inportb(0x64) & 2)==0)
      {
        return;
      }
}

bool mouse_readable() {
    return (inportb(0x64) & 1)==1;
}

// Wait for bit 0 of port 64 to go high
void mouse_wait_read() {
  for (volatile int i=mouse_busywait_time;i>0;i--)
      if(mouse_readable())
      {
        return;
      }
}

inline void mouse_write(int a_write) //unsigned char
{
  //Wait to be able to send a command
  mouse_wait_write();
  //Tell the mouse we are sending a command
  outportb(0x64, 0xD4);
  //Wait for the final part
  mouse_wait_write();
  //Finally write
  outportb(0x60, a_write);
}

int mouse_read()
{
  //Get's response from mouse
  mouse_wait_read();
  return inportb(0x60);
}

void mouse_install()
{
  //Enable the auxiliary mouse device
  mouse_wait_write();
  outportb(0x64, 0xA8);
  
  //Reset the mouse
  //print("Reset mouse: ");
  mouse_write(0xFF);
  int c;
  do {
    c=mouse_read();
    //print((uint64_t)c); print(" ");
  } while (c!=0xAA);
  //println();
  
  //Tell the mouse to use default settings
  mouse_write(0xF6);
  mouse_read();  //0xFA Acknowledge
 
  //Enable packet streaming (turns mouse on, otherwise it doesn't work)
  mouse_write(0xF4);
  mouse_read();  //0xFA Acknowledge
}


UserEventSource::UserEventSource()
{
    print("Mouse setup...\n");
    mouse_install();

    mouse.x=300;
    mouse.y=300;
    mouse.buttons=0;
    mouse.scroll=0;
    mouse.modifiers=0;
}


bool UserEventSource::probeMouse(void)
{
    bool hasData=false;
    // See: https://wiki.osdev.org/Mouse_Input

    for (int leash=5;leash>0 && mouse_readable();leash--) {
        int flags=inportb(0x60);
        //print((uint64_t)flags); print("=m ");
        
        enum { flag_overflow=0xC0, flag_ysign=0x20, flag_xsign=0x10, 
               flag_valid=0x8, flag_buttons=0x7 };
        if (flags&flag_valid) 
        { // looks like a valid start byte: grab the rest of the bytes
            int dx=inportb(0x60);//mouse_read();
            if (!mouse_readable()) break;
            int dy=inportb(0x60);//mouse_read();
            
            if (flags==dx && flags==dy) break; // invalid (hit keyboard data?)
            if (flags&flag_overflow) continue;  // probably bad
            
            if (flags&flag_ysign) dy+=-256;
            if (flags&flag_xsign) dx+=-256;
            
            mouse.x += dx;
            mouse.y -= dy; // mouse +Y goes up (?!)
            mouse.buttons = flags & flag_buttons;
            hasData=true;

            leash=2; //<- keep reading more data if we keep getting more data
        }
    }
    return hasData;
}
        

bool UserEventSource::waitForEvent(int delayMS,UserEventHandler &handler)
{
    
    EFI_EVENT timer;
    UEFI_CHECK(ST->BootServices->CreateEvent(
        EFI_EVENT_TIMER, 0, 0, 0, &timer));
    UEFI_CHECK(ST->BootServices->SetTimer(timer,TimerRelative,
        10000*delayMS));
    
    enum {nEvent=2};
    EFI_EVENT events[nEvent]={
        ST->ConIn->WaitForKey, // keyboard is event 0
        timer // timer is event 1
    };
    
    UINTN index=0;
    UEFI_CHECK(ST->BootServices->WaitForEvent(nEvent,events,&index));
    
    if (index!=0) {
        // Check for mouse input
        if (probeMouse()) {
            handler.handleMouse(mouse);
        }
    }
    
    // Clean up timer event regardless
    ST->BootServices->CloseEvent(timer);
    
    switch(index) {
    case 0: { // keyboard event
            EFI_INPUT_KEY key;
            UEFI_CHECK(ST->ConIn->ReadKeyStroke (ST->ConIn, &key));
            KeyTyped kt;
            kt.scancode=key.ScanCode;
            kt.unicode=key.UnicodeChar;
            kt.modifiers=0; //<- get modifiers!
            handler.handleKeystroke(kt);
            return true;
        }
    case 1: // timer expired, nothing happened
        return false;
        
    default:
        panic("Unknown index returned from WaitForEvent!",index);
    }
    return false;
}




/// For debugging, the event handler just prints the events
class PrintHandler : public UserEventHandler {
public:
    virtual void handleKeystroke(const KeyTyped &key) {
        print("Keystroke ");
        print((int)key.unicode);
        
        print("  scancode="); print((int)key.scancode); 
        print("  modifiers="); print((int)key.modifiers);
        println();
    }
    virtual void handleMouse(MouseState &mouse) {
        print("Mouse XY  ");
        print((int)mouse.x); print(", ");
        print((int)mouse.y); 
        
        print("  scroll="); print((int)mouse.scroll); 
        print("  buttons="); print((int)mouse.buttons);
        print("  modifiers="); print((int)mouse.modifiers);
        println();
    }
};

void test_UI(void) {
    UserEventSource src;
    PrintHandler handler;
    println("Waiting for events");
    while(true) {
        src.waitForEvent(1,handler);
    }
}




