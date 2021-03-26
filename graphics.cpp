/* 
  Graphics functions for GLaDOS.
  
  Built around the UEFI Graphics Output Protocol interface: 
     https://github.com/tianocore/edk2/blob/master/MdePkg/Include/Protocol/GraphicsOutput.h
  
  Dr. Orion Lawlor and the UAF CS 321 class, 2021-03 (Public Domain)
*/
#include "GLaDOS/GLaDOS.h"
#include "GLaDOS/gui/graphics.h"
#include "GLaDOS/gui/window.h"

/* Baked-in PNG images: */
#include "GLaDOS/gui/image.h"

static KernelBuiltinImages *builtinImages=0;
    
/// Return a reference to a single copy of the loaded images.
///  (Avoids global initialization order problems by doing this delayed.)
const KernelBuiltinImages &KernelBuiltinImages::load()
{
    if (!builtinImages) 
    { // finally call actual constructor
        builtinImages=new KernelBuiltinImages();
    }
    return *builtinImages;
}

// These are the header-ified copies of the PNG source images:
#include "GLaDOS/gui/img/Mouse.h"
#include "GLaDOS/gui/img/Courier.h"

KernelBuiltinImages::KernelBuiltinImages()
    :mouse(Mouse_png,Mouse_png_len),
     courier(Courier_png,Courier_png_len)
{}

// PNG decode library from https://github.com/lvandeve/lodepng
//  (modified to work in GLaDOS)
#include "lodepng.h"

PngImage::PngImage(const void *imageData,uint64_t imageDataBytes)
    :GraphicsOutput<BGRAPixel>(0,0,0,0)
{
    unsigned char *pixels=0;
    unsigned lwid=0, lht=0;
    unsigned error=lodepng_decode32(
        &pixels,&lwid,&lht,
        (const unsigned char *)imageData,(size_t)imageDataBytes
    );
    if (error!=0) panic("Lodepng decode error",error);
    
    wid=lwid; ht=lht;
    pixelsPerRow=wid;
    frame=Rect(0,wid,0,ht);
    framebuffer=(BGRAPixel *)pixels;
}

PngImage::~PngImage()
{
    gfree(framebuffer);
}

/* Font rendering */
static Font *defaultFont=0;

/// Load up the default font
Font &Font::load()
{
    if (!defaultFont) {
        defaultFont=new Font();
    }
    return *defaultFont;
}

/// Load up a font by this name
Font::Font(const char *name,int size)
    :chars(KernelBuiltinImages::load().courier),
     letterBox(0,10,0,15),
     fixedWidth(9)
{
    /// FIXME: check the name and size, don't just hardcode courier
}

int Font::charWidth(int c) const
{
    return fixedWidth;
}

/// Render text to this location in this image.
///  Returns the new text start point (in pixels).
Point Font::draw(const StringSource &text,const Point &initialStart,
        const ScreenPixel &color,GraphicsOutput<ScreenPixel> &gfx) const
{
    Point start=initialStart;
    const int stepX=16, stepY=16; //<- pixels per char in chars image
    const int wrapX=16; //<- chars per row in chars image
    Point corner(0,-12); // shift from start point (on baseline) to charBox topleft
    
    // Loop over the chars in the StringSource:
    ByteBuffer buf; int index=0;
    while (text.get(buf,index++)) 
        for (unsigned char c:buf) {
            if (c>=128) { // uh oh, unicode!  Just draw a black box.
                gfx.fillRect(letterBox.shifted(start+corner),color);
                start.x+=charWidth('m');
            }
            else if (c>=32) { // printable ASCII
                // Figure out where we're at in the font image
                int row=c/wrapX-2; // font image skips 2 rows of control chars
                int col=c%wrapX;
                Point srcStart(stepX*col,stepY*row);
                chars.colorizeTo(
                    letterBox.shifted(srcStart), // src in font image
                    color,
                    letterBox.shifted(start+corner), // dest rect in gfx
                    gfx);
                start.x+=charWidth(c);
            }
            else if (c=='\n') { // newline
                start.x=initialStart.x;
                start.y+=letterBox.ht();
            }
            else if (c=='\t') { // hard tab
                start.x+=4*charWidth(' ');
            }
            else { // unknown control char, ignore?
            }
            
            // Check for char-by-char text wrap: if so, fake a newline.
            if (start.x+letterBox.wid()>gfx.wid) { 
                start.x=initialStart.x;
                start.y+=letterBox.ht();
            }
        }
    
    return start;
}


/* UEFI framebuffer access */
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
    GraphicsOutput<ScreenPixel> &gfx=graphics.out;
    
    print("Size of framebuffer: ");
    print((int)(gfx.wid*gfx.ht*sizeof(ScreenPixel)));
    println();
    
    const KernelBuiltinImages &imgs=KernelBuiltinImages::load();
    const PngImage &mouse=imgs.mouse;
    mouse.printSize();
    gfx.fillRect(Rect(0,256,0,96),0xffffff); // white background
    mouse.blendTo(mouse.frame,mouse.frame,gfx); // mouse
    
    Font &f=Font::load();
    f.draw("Hello world!",Point(50,50),0,gfx);
    /*
    const PngImage &font=imgs.courier;
    Rect letterBox(0,10, 0,16);
    
    for (int bloc=0;bloc<260;bloc+=7)
        font.colorizeTo(letterBox.shifted(32,32), // source rect (letter)
            0, // font color (black)
            letterBox.shifted(bloc,0), // dest rect (where it goes)
            gfx); // all the chars of the font
            */
    
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

bool run_gui=true;

/// Draw a text terminal
class ProcessTerminal : public Process {
public:
    Font &font;
	ProcessTerminal(Window &window_)
	    :Process(window_), font(Font::load())
	{}
	
	char prompt[100]={'>',' ',0};
	char cmdline[100]={0};
	char output[1024]={0};
	int cursor=0; //<- index into cmdline array above
	ScreenPixel background=0x00001f;
	ScreenPixel persistentBackground=0x00001f;
	ScreenPixel foreground=0xffffff;
	
	virtual void drawBackground(void)
	{
	    gfx.fillRect(gfx.frame,background);
	    // Incrementally alpha blend to the persistent background color
	    background.blend(persistentBackground,50);
	}
	
	Point endOfPrompt;
	virtual void drawText(void)
	{
	    endOfPrompt=font.draw(prompt,Point(0,gfx.ht*1/4),foreground,gfx);
	    font.draw(cmdline,endOfPrompt,foreground,gfx);
	    font.draw(output,Point(0,gfx.ht*2/4),foreground,gfx);
	}
	
	virtual void drawCursor(void)
	{
	    int cursorColor=0xff0000;
	    // See the cursor onscreen:
	    Rect cursorShape(0,1,-12,2); // thin classy rect
	    //Rect cursorShape(1,3,-12,2); // thicker rect
	    //Rect cursorShape(1,10,-12,2); // giant block (UNIX or DOS style)
	    
	    Point cursorPosition(cursor*font.charWidth(' '),0);
	    gfx.fillRect(cursorShape.shifted(endOfPrompt+cursorPosition),
	        cursorColor);
	}
	
	virtual void run(void)
	{
	    drawBackground();
	    drawText();
	    drawCursor();
	}
	
	// Visual indication of "oops", a minor error
	virtual void oops(void) 
	{
        background=0xDF0000;
	}
	
    virtual void handleMouse(MouseState &mouse) {
        if (mouse.buttonDown(0))
        {
            // FIXME: handle mouse clicks, allow command line edit
        }
    }
    virtual void executeCommand(char *cmd) {
        if (0==strcmp(cmd,"help")) {
            strcpy(output,"Commands: ls, help, exit");
        }
        else if (0==strcmp(cmd,"ls")) {
            strcpy(output,"Listing!\nWith newlines!");
        }
        else if (0==strcmp(cmd,"strtest")) {
            int out=strcmp("f","foo");
            if (out>0) strcpy(output,"positive");
            else if (out<0) strcpy(output,"negative");
            else strcpy(output,"zero");
        }
        else if (0==strcmp(cmd,"exit")) {
            strcpy(output,"GOODBYE!");
            run_gui=false;
        }
        else if (1==strlen(cmd)) {
            strcpy(output,"Running goofy one-char command...");
            clear_screen();
            handle_command(cmd[0]);
            pause(); //<- let user read the command output, then back to GUI
        }
        else {
            strcpy(output,"UNRECOGNIZED COMMAND -- ERROR\n");
            oops();
        }
    }
    virtual void handleKeystroke(const KeyTyped &key) {
        if (key.scancode==4) { // left arrow
            cursor--;
            if (cursor<0) { cursor=0; oops(); }
        }
        else if (key.scancode==3) { // right arrow
            if (cmdline[cursor]!=0) cursor++; else oops();
        }
        else if (key.unicode=='\r') { // newline
            executeCommand(cmdline);
            strncpy(cmdline,"",sizeof(cmdline));
            cursor=0;
            return;
        }
        else if (key.unicode==8) { // backspace
            cursor--;
            if (cursor<0) { cursor=0; oops(); }
            cmdline[cursor]=0;
        }
        else if (cursor+1<sizeof(cmdline))
        {
            cmdline[cursor++]=key.unicode;
        }
        else 
            background=0xff0000; //<- screen goes red once buffer is full
        
        // FIXME: handle newlines, etc
    }
};

/// Draw a bouncing ball
class ProcessBall : public Process {
public:
	ProcessBall(Window &window_)
	    :Process(window_)
	{}
	
	char text[100]={0};
	int animation=0;
	int background=0xff008f;
	Point userClick;
	virtual void run(void)
	{
	    gfx.fillRect(gfx.frame,background);
	    
	    Point circle(50,animation%gfx.ht);
	    if (userClick.x!=0) circle=userClick;
	    gfx.drawBlendCircle(circle.x,circle.y,40,0x0000ff); 
	
	    //window.move(Point(2,0)); // move window to the right
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
    Window *wa=new Window("GLaTerm",Rect(50,650,450,550));
    winmgr.add(wa);
    Process *a=new ProcessTerminal(*wa);
    a->next=a;
    a->prev=a;
    cur=a;
    
    Window *wb=new Window("Gradient",Rect(300,700,100,400));
    winmgr.add(wb);
    Process *b=new ProcessGradient(*wb);
    make_process_runnable(b);
        
    // Connect to keyboard and mouse:
    UserEventSource src;
    
    run_gui=true;
    while (run_gui) 
    {
    // Run some processes
        cur->run();
        cur=cur->next;
        
    // Grab keyboard and mouse events
        src.waitForEvent(5,winmgr);
    
    // Update the screen
        winmgr.drawScreen(framebuffer);
    }
    
    // Clean up the text console
    clear_screen();
}






