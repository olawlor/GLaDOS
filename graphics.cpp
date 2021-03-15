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
    GraphicsOutput() {
        gfx=get_graphics();
        mode=gfx->Mode;
        info=mode->Info;
        wid=info->HorizontalResolution;
        ht=info->VerticalResolution;
        frame=Rect(0,wid,0,ht);
        framebuffer=(Pixel *)mode->FrameBufferBase;
    }
    
    /// Get a read-write reference to pixel (x,y).  This location must be in bounds.
    Pixel &at(int x,int y) { return framebuffer[x+y*wid]; }
    
    
    
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
    
    /// Draw a circle at this cx,cy center, radius, and color.
    void drawWindow(const WindowColors &colors,const Rect &r)
    {
        fillRect(r,0x202020);
        outlineRect(r,1,colors.border);
        fillRect(windowToTitlebar(r),colors.titlebar);
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
    
    /// Print background info about the graphics resolution and format
    void printInfo(void) {
        print("mode "); print((int)mode->Mode); 
           print(" of "); print((int)mode->MaxMode); print("\n");
        
        print(wid); print(" pixels in X; ");
        print(ht); print(" pixels in Y\n");
        
        print(info->PixelFormat); print(" pixel format (1==BGR_)\n");
        print((uint64_t)framebuffer); print(" framebuffer base address\n");
    }

    /// It's useful to know the size of the output, so make these public:
    int wid,ht;
    Rect frame;
    Pixel *framebuffer;
    
private:    
    EFI_GRAPHICS_OUTPUT_PROTOCOL *gfx;
    EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE *mode;
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *info;
};


// Draw a HAL-style glowing eye
void draw_HAL()
{
    GraphicsOutput<BGRAPixel> graphics;
    int sz=64; // radius of the disk
    int cx=graphics.wid-sz+1, cy=sz+1;
    graphics.drawBlendCircle(cx,cy,sz,BGRAPixel(255,0,0)); // big red disk
    graphics.drawBlendCircle(cx,cy,12,BGRAPixel(255,255,0)); // yellow middle dot
}

void print_graphics()
{
    GraphicsOutput<BGRAPixel> graphics;
    graphics.printInfo();
}


void test_graphics()
{
    GraphicsOutput<BGRAPixel> graphics;
    draw_HAL();
    graphics.drawWindow(defaultColors,Rect(100,400,300,500));
    graphics.drawWindow(defaultColors,Rect(350,700,100,400));
    
    
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






