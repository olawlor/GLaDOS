/* 
  Graphics functions for GLaDOS.
  
  Built around the UEFI Graphics Output Protocol interface: 
     https://github.com/tianocore/edk2/blob/master/MdePkg/Include/Protocol/GraphicsOutput.h
  
  Dr. Orion Lawlor and the UAF CS 321 class, 2021-03 (Public Domain)
*/
#include "GLaDOS/GLaDOS.h"
#include "GLaDOS/gui/graphics.h"
#include "GLaDOS/gui/window.h"


EFI_GRAPHICS_OUTPUT_PROTOCOL *get_graphics(void)
{
    static EFI_GRAPHICS_OUTPUT_PROTOCOL *gfx=0;
    if (!gfx) { // Locate the graphics protocol
        static EFI_GUID gfx_guid= EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
        UEFI_CHECK(ST->BootServices->LocateProtocol(&gfx_guid,0,(void **)&gfx));
    }
    return gfx;
}

class UEFIGraphics {
private:
    EFI_GRAPHICS_OUTPUT_PROTOCOL *gfx;
    EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE *mode;
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *info;
    
public:
    GraphicsOutput<ScreenPixel> out;
    
    UEFIGraphics() 
        :gfx(get_graphics()),
         mode(gfx->Mode),
         info(mode->Info),
         out(info->HorizontalResolution,
            info->VerticalResolution,
            info->HorizontalResolution,
            (ScreenPixel *)mode->FrameBufferBase)
    {}
    
    /// Print background info about the graphics resolution and format
    void printInfo(void) {
        print("mode "); print((int)mode->Mode); 
           print(" of "); print((int)mode->MaxMode); print("\n");
        
        print(out.wid); print(" pixels in X; ");
        print(out.ht); print(" pixels in Y\n");
        
        print(info->PixelFormat); print(" pixel format (1==BGR_)\n");
        print((uint64_t)out.framebuffer); print(" framebuffer base address\n");
    }
    
};



void print_graphics()
{
    UEFIGraphics graphics;
    graphics.printInfo();
    
    print("Size of framebuffer: ");
    print((int)(graphics.out.wid*graphics.out.ht*sizeof(ScreenPixel)));
    println();
}


volatile int dont_optimize=0;
/// Wait for about this many milliseconds
///  (used to slow down animation to be a visible speed)
void delay(int ms) {
    for (int wait=0;wait<ms;wait++)
        for (int inner=0;inner<500000;inner++)
            dont_optimize++;
}




/// A Process gets CPU time.
///   Child classes override the run and event handler methods.
class Process : public UserEventHandler {
public:
	/// Scheduler queue entry (circular doubly-linked list of processes)
	Process *next; 
	Process *prev; 
	
	/// Graphical window onscreen
	Window &window;
	/// Where we draw our graphical output:
	GraphicsOutput<ScreenPixel> &gfx;
	
	/// Process info:
	
	/// Signal handler list?
	
	/// I/O bound or CPU bound?  -> process priority!
	
	
	/// Run method:
	virtual void run(void) {}
	
	Process(Window &window_)
	    :window(window_),
	    gfx(window.offscreen)
	{
		next=prev=0;
		window.handler=this; // we handle events for our window
	}
	
	virtual ~Process() {}
};

/// This is the next runnable process:
Process *cur=0;
Process *make_process_runnable(Process *nu)
{
	nu->next=cur->next;
	nu->prev=cur;
	
	cur->next->prev=nu; 
	cur->next=nu;
	
	return nu;
}

void end_process(Process *doomed)
{
	// Remove self from doubly-linked list
	doomed->prev->next=doomed->next; // remove me from the list
	doomed->next->prev=doomed->prev;
	if (doomed==cur) cur=doomed->next;
	// print(doomed->name); print("is done\n");
	delete doomed;
}



/// Draw a bouncing ball
class ProcessBall : public Process {
public:
	ProcessBall(Window &window_)
	    :Process(window_)
	{}
	
	int animation=0;
	int background=0xff008f;
	Point userClick;
	virtual void run(void)
	{
	    gfx.fillRect(gfx.frame,background);
	    
	    Point circle(50,animation%gfx.ht);
	    if (userClick.x!=0) circle=userClick;
	    gfx.drawBlendCircle(circle.x,circle.y,40,0x0000ff); 
	
	    window.move(Point(2,0)); // move window to the right
		animation+=10; // change brightness per frame
	}
	
    virtual void handleMouse(MouseState &mouse) {
        if (mouse.buttonDown(0))
            userClick=mouse;
    }
    virtual void handleKeystroke(const KeyTyped &key) {
        background=0xff0000; //<- background goes red, just a visible response
    }
};

/// Draw a color gradient
class ProcessGradient : public Process {
public:
	ProcessGradient(Window &window_)
	    :Process(window_)
	{}
	
	int blue=0;
	virtual void run(void)
    {
		for_xy_in_Rect(gfx.frame) // x,y location of a pixel
		    gfx.at(x,y)=ScreenPixel(x*255/gfx.wid,y*255/gfx.ht,blue);
	}
	
    virtual void handleKeystroke(const KeyTyped &key) {
        blue=255; //<- just a visible response
    }
};



void test_graphics()
{
    UEFIGraphics graphics;
    GraphicsOutput<ScreenPixel> &framebuffer=graphics.out;
    WindowManager winmgr(framebuffer);
    
    // Make a few little hardcoded processes, with their own windows:
    Window *wa=new Window("A",Rect(100,400,300,500));
    winmgr.add(wa);
    Process *a=new ProcessBall(*wa);
    a->next=a;
    a->prev=a;
    cur=a;
    
    Window *wb=new Window("B",Rect(300,700,100,400));
    winmgr.add(wb);
    Process *b=new ProcessGradient(*wb);
    make_process_runnable(b);
        
    // Connect to keyboard and mouse:
    UserEventSource src;
    
    while (1) 
    {
    // Run some processes
        cur->run();
        cur=cur->next;
        
    // Grab keyboard and mouse events
        src.waitForEvent(20,winmgr);
    
    // Update the screen
        winmgr.drawScreen(framebuffer);
    }
}






