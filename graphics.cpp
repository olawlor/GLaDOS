/* 
  Graphics functions for GLaDOS.
  
  Built around the UEFI Graphics Output Protocol interface: 
     https://github.com/tianocore/edk2/blob/master/MdePkg/Include/Protocol/GraphicsOutput.h
  
  Dr. Orion Lawlor and the UAF CS 321 class, 2021-03 (Public Domain)
*/
#include "GLaDOS/GLaDOS.h"

EFI_GRAPHICS_OUTPUT_PROTOCOL *get_graphics(void)
{
    static EFI_GRAPHICS_OUTPUT_PROTOCOL *gfx=0;
    if (!gfx) { // Locate the graphics protocol
        static EFI_GUID gfx_guid= EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
        UEFI_CHECK(ST->BootServices->LocateProtocol(&gfx_guid,0,(void **)&gfx));
    }
    return gfx;
}

// Default pixel type on UEFI seems to be BGRA
struct BGRAPixel {
    UINT8 B;
    UINT8 G;
    UINT8 R;
    UINT8 A;
    
    BGRAPixel(int r,int g,int b,int a=0) {
        R=r; G=g; B=b; A=a;
    }
    BGRAPixel(int HTMLcolor) {
        B=HTMLcolor&0xff;
        G=(HTMLcolor>>8)&0xff;
        R=(HTMLcolor>>16)&0xff;
        A=(HTMLcolor>>24)&0xff;
    }
    
    /// Blend a new color on top of us.
    ///   alpha==0 means leave under unchanged.
    ///   alpha==255 means under=over
    void blend(const BGRAPixel &over,int alpha=255)
    {
        R=(R*(256-alpha)+over.R*(1+alpha))>>8; // bitshift instead of divide by 256
        G=(G*(256-alpha)+over.G*(1+alpha))>>8;
        B=(B*(256-alpha)+over.B*(1+alpha))>>8;
    }
};

struct WindowColors {
    BGRAPixel titlebar; // behind text on the titlebar
    BGRAPixel border; // hard border around window
};
const WindowColors defaultColors={ 0x685556, 0xff0000 };

/// A Span is a range of values, from lo to hi-1.
///  It's used inside rect to avoid duplicate min/max and x/y code.
struct Span {
    int lo; // first element in the span
    int hi; // last+1 element in the span
    Span(int l,int h) {lo=l; hi=h;}
    Span() { lo=hi=0; }
    
    /// Return the size of this span
    int size(void) const { return hi-lo; }
    
    /// Return the middle (rounded down) of the span
    int middle(void) const { return (lo+hi)/2; }
    
    /// Intersect this span: return a Span containing only pixels in both spans.
    Span intersect(const Span &other) const {
        return Span(lo>other.lo?lo:other.lo,
            hi<other.hi?hi:other.hi);
    }
};

/// A Rect is a 2D block of pixels.
struct Rect {
    Span X;
    Span Y;
    
    /// Empty rectangle
    Rect() {}
    
    /// Make a rectangle from (minX,minY) to (maxX,maxY):
    Rect(int minX,int maxX, int minY,int maxY)
        :X(minX,maxX), 
         Y(minY,maxY) 
         {}
    
    /// Make a rectangle centered at (x,y) with radius r
    Rect(int cenX,int cenY,int radius) 
        :X(cenX-radius,cenX+radius),
         Y(cenY-radius,cenY+radius)
         {}
    
    /// Make a rectangle from these two Spans:
    Rect(const Span &spanX,const Span &spanY)
        :X(spanX), Y(spanY) {}
    
    /// Intersect these rectangles: return a Rectangle containing only pixels inside both.
    Rect intersect(const Rect &r) const {
        return Rect(X.intersect(r.X), Y.intersect(r.Y));
    }
    
    /// Return a shifted rectangle, moved by this far
    Rect shift(int dx,int dy) const {
        return Rect(X.lo+dx,X.hi+dx, Y.lo+dy,Y.hi+dy);
    }
};

/// This macro loops over the pixels in a rectangle,
///   making (x,y) for each pixel in the rectangle.
#define for_xy_in_Rect(r) for (int y=r.Y.lo;y<r.Y.hi;y++) for (int x=r.X.lo;x<r.X.hi;x++)

// Given a rect for a window, return the rect for the titlebar
Rect windowToTitlebar(const Rect &w) 
{
    return Rect(w.X.middle(),w.X.hi,
        w.Y.lo-32, w.Y.lo);
}


template<typename Pixel>
class GraphicsOutput {
public:
    // It's useful to know the size of the output, so make these public:

    /// Size of the framebuffer, in pixels
    int wid,ht; 
    /// Framebuffer dimensions as a Rect, for clipping
    Rect frame;
    /// This is where we draw our pixels
    Pixel *framebuffer;
    
    GraphicsOutput(int wid_,int ht_,Pixel *framebuffer_)
        :wid(wid_), ht(ht_), frame(0,wid,0,ht),
         framebuffer(framebuffer_)
    {}
    
    /// Get a read-write reference to pixel (x,y).  This location must be in bounds.
    Pixel &at(int x,int y) { return framebuffer[x+y*wid]; }
    
    /// Copy our pixels to another output device.
    void copyTo(const Rect &src,const Rect &dest,GraphicsOutput<Pixel> &target)
    {
        // Shift is the transform from src coords to dest coords
        int shiftX=dest.X.lo-src.X.lo;
        int shiftY=dest.Y.lo-src.Y.lo;
        
        // Clipping
        Rect destf=dest.intersect(target.frame);
        Rect srcf=src.intersect(frame);
        Rect copy=destf.intersect(srcf.shift(shiftX,shiftY));
        
        // Data copy:
        for_xy_in_Rect(copy) target.at(x,y)=at(x-shiftX,y-shiftY);
    }
    
    
    /// Draw a box at this cx,cy center, side sizes, and color.
    void drawCenteredBox(int cx,int cy,int size,const Pixel &color)
    {
        Rect r(cx,cy,size);
        Rect rf=r.intersect(frame);
        for_xy_in_Rect(rf) at(x,y)=color;
    }
    
    /// Fill a rectangle with a solid color
    void fillRect(const Rect &r,const Pixel &color)
    {
        Rect rf=r.intersect(frame);
        for_xy_in_Rect(rf) at(x,y)=color;
    }
    
    /// Outline a rectangle
    void outlineRect(const Rect &r,int wide,const Pixel &color)
    {
        // Top strip:
        fillRect(Rect(r.X.lo,r.X.hi,r.Y.lo,r.Y.lo+wide),color);
        // Left strip:
        fillRect(Rect(r.X.lo,r.X.lo+wide,r.Y.lo+wide,r.Y.hi-wide),color);
        // Right strip:
        fillRect(Rect(r.X.hi-wide,r.X.hi,r.Y.lo+wide,r.Y.hi-wide),color);
        // Bottom strip:
        fillRect(Rect(r.X.lo,r.X.hi,r.Y.hi-wide,r.Y.hi),color);
    }
    
    void shadowRect(const Rect &r)
    {
        Rect shadow=r.shift(2,2);
        fillRect(Rect(r.X.hi,shadow.X.hi,shadow.Y.lo,shadow.Y.hi),0); // right side
        fillRect(Rect(shadow.X.lo,shadow.X.hi,r.Y.hi,shadow.Y.hi),0); // bottom side
    }
    
    /// Draw window decorations for a window of this size
    void drawWindow(const WindowColors &colors,const Rect &r)
    {
        shadowRect(r); // content area of window
        
        Rect title=windowToTitlebar(r);
        shadowRect(title);
        
        //fillRect(r,0x202020);// content area (application buffer fills this now)
        outlineRect(r,1,colors.border);
        fillRect(title,colors.titlebar);
    }
    
    /// Draw a blended circle at this cx,cy center, radius, and color.
    void drawBlendCircle(int cx,int cy,int radius,const Pixel &color)
    {
        Rect r(cx,cy,radius);
        Rect rf=r.intersect(frame);
        for_xy_in_Rect(rf) 
        {
            int r2=(x-cx)*(x-cx)+(y-cy)*(y-cy);
            if (r2<radius*radius) {
                int alpha=255-sqrtf(r2)*255/radius;
                at(x,y).blend(color,alpha);
            }
        }
    }
};

// Stores our pixel data in an offscreen buffer
template <class Pixel>
class OffscreenGraphics : public GraphicsOutput<Pixel>
{
public:
    OffscreenGraphics(int wid_,int ht_)
        :GraphicsOutput<Pixel>(wid_,ht_,(Pixel *)galloc(sizeof(Pixel)*wid_*ht_))
    {}
    ~OffscreenGraphics() {
        gfree(this->framebuffer); this->framebuffer=0;
    }
};



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
		
		if (name[0]=='A') {
		    enum {ncolors=4};
		    if (++color>=ncolors) color=0;
		    const int colors[ncolors]={0xff008f,0xff007f,0xff006f,0xff007f};
		    
		    offscreen.fillRect(offscreen.frame,colors[color]);
		    
		    // blue gradient circle
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
	    gfx.drawWindow(defaultColors,window); // window decorations
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
    
    while (1) 
    {
        cur->run();
        cur=cur->next;
    
    // Redraw everything offscreen:
        // Background of desktop
        backbuffer.fillRect(backbuffer.frame,0x808080);
        
        draw_HAL(backbuffer);
        
        a->draw(backbuffer);
        b->draw(backbuffer);
        
        // FIXME: draw the mouse location!
        
        backbuffer.copyTo(backbuffer.frame,framebuffer.frame,framebuffer);
        
        delay(100);
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






