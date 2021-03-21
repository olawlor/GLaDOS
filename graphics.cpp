/* 
  Graphics functions for GLaDOS.
  
  Built around the UEFI Graphics Output Protocol interface: 
     https://github.com/tianocore/edk2/blob/master/MdePkg/Include/Protocol/GraphicsOutput.h
  
  Dr. Orion Lawlor and the UAF CS 321 class, 2021-03 (Public Domain)
*/
#include "GLaDOS/GLaDOS.h"
#include "GLaDOS/gui/graphics.h"

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
    GraphicsOutput<BGRAPixel> out;
    
    UEFIGraphics() 
        :gfx(get_graphics()),
         mode(gfx->Mode),
         info(mode->Info),
         out(info->HorizontalResolution,
            info->VerticalResolution,
            info->HorizontalResolution,
            (BGRAPixel *)mode->FrameBufferBase)
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


struct WindowColors {
    BGRAPixel titlebar; // behind text on the titlebar
    BGRAPixel border; // hard border around window
};
const WindowColors defaultColors={ 0x685556, 0xff0000 };


/// Given a rect for a window, return the rect for the titlebar
Rect windowToTitlebar(const Rect &w) 
{
    return Rect(w.X.middle(),w.X.hi,
        w.Y.lo-32, w.Y.lo);
}
    
/// Draw window decorations for a window of this size
void drawWindow(GraphicsOutput<BGRAPixel> &gfx,const WindowColors &colors,const Rect &r)
{
    gfx.shadowRect(r); // content area of window
    
    Rect title=windowToTitlebar(r);
    gfx.shadowRect(title);
    
    gfx.outlineRect(r,1,colors.border);
    gfx.fillRect(title,colors.titlebar);
}




// Draw a HAL-style glowing eye
void draw_HAL(GraphicsOutput<BGRAPixel> &gfx)
{
    int sz=64; // radius of the disk
    int cx=gfx.wid-sz+1, cy=sz+1;
    gfx.drawBlendCircle(cx,cy,sz,BGRAPixel(255,0,0)); // big red disk
    gfx.drawBlendCircle(cx,cy,12,BGRAPixel(255,255,0)); // yellow middle dot
}

void print_graphics()
{
    UEFIGraphics graphics;
    graphics.printInfo();
    
    print("Size of framebuffer: ");
    print((int)(graphics.out.wid*graphics.out.ht*sizeof(BGRAPixel)));
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



class Process {
public:
	/// Scheduler queue entry (circular doubly-linked list of processes)
	Process *next; 
	Process *prev; 
	
	/// Process info:
	const char *name;
	
	/// Signal handler list?
	
	/// I/O bound or CPU bound?  -> process priority!
	
	Rect window; // dimensions of window onscreen
	OffscreenGraphics<BGRAPixel> offscreen; // window's contents
	
	int color;
	int animation=0;
	
	/// Run method:
	virtual void run(void) {
		//print("Running "); print(name); print("\n");
		
		if (name[0]=='A') { // bouncing blue circle
		    offscreen.fillRect(offscreen.frame,0xff008f);
		    
		    offscreen.drawBlendCircle(50,animation%offscreen.ht,40,0x0000ff); 
		
		    window=window.shift(2,0); // move window to the right
		}
		else { // B: red/green gradient
		    for_xy_in_Rect(offscreen.frame) // x,y location of a pixel
		        offscreen.at(x,y)=BGRAPixel(x*255/offscreen.wid+animation,y*255/offscreen.ht,0); 
		    
		}
		animation+=10; // change brightness per frame
	}
	
	/// Draw method: update window display onscreen
	virtual void draw(GraphicsOutput<BGRAPixel> &gfx) {
	    drawWindow(gfx,defaultColors,window); // window decorations
	    offscreen.copyTo(offscreen.frame,window,gfx); // contents
	}
	
	Process(const char *name_,Rect r)
	    :offscreen(r.X.size(),r.Y.size())
	{
		next=0;
		name=name_;
		window=r;
	}
	
	virtual ~Process() {}
};

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
	print(doomed->name); print("is done\n");
	delete doomed;
}


int desktopColor=0x808080;

/// Draw stuff onscreen as events happen!
class GraphicsEventHandler : public UserEventHandler {
public:
    virtual void handleKeystroke(const EFI_INPUT_KEY &key) {
        //if (key.UnicodeChar=='r') 
        desktopColor=0xff0000;
    }
    
    EFI_ABSOLUTE_POINTER_STATE mouse;
    virtual void handleMouse(EFI_ABSOLUTE_POINTER_STATE &newMouse) {
        if (newMouse.CurrentX<0) newMouse.CurrentX=0;
        if (newMouse.CurrentY<0) newMouse.CurrentY=0;
        mouse=newMouse;
    }
}; 



void test_graphics()
{
    UEFIGraphics graphics;
    GraphicsOutput<BGRAPixel> &framebuffer=graphics.out;
    OffscreenGraphics<BGRAPixel> backbuffer(framebuffer.wid,framebuffer.ht);
    
    Process *a=new Process("A",Rect(100,400,300,500));
    a->next=a;
    a->prev=a;
    cur=a;
    
    Process *b=new Process("B",Rect(300,700,100,400));
    make_process_runnable(b);
    
    GraphicsEventHandler guiHandler;
    UserEventSource src;
    
    while (1) 
    {
        cur->run();
        cur=cur->next;
        
    // Grab keyboard and mouse events
        src.waitForEvent(10,guiHandler);
    
    // Redraw everything offscreen:
        // Background of desktop
        backbuffer.fillRect(backbuffer.frame,desktopColor);
        
        draw_HAL(backbuffer);
        
        a->draw(backbuffer);
        b->draw(backbuffer);
        
        int cursorX=guiHandler.mouse.CurrentX;
        int cursorY=guiHandler.mouse.CurrentY;
        int cursorSize=20; // pixels onscreen
        int cursorScale=1; // speed of movement
        backbuffer.fillRect(Rect(cursorX/cursorScale,cursorY/cursorScale,cursorSize/2),0);
        
        backbuffer.copyTo(backbuffer.frame,framebuffer.frame,framebuffer);
    }
    
/*
    // Basic manual graphics:
    EFI_GRAPHICS_OUTPUT_PROTOCOL *gfx=0;
	EFI_GUID gfx_guid= EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
    ST->BootServices->LocateProtocol(&gfx_guid,0,(void **)&gfx);
    EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE *mode=gfx->Mode;
    

    int *framebuffer=(int *)mode->FrameBufferBase;
    int wid=mode->Info->HorizontalResolution;
    for (int y=0;y<500;y++) 
    for (int x=0;x<500;x++) 
        framebuffer[x+y*wid]=0x00ff00; // <- big green box
*/        
}






