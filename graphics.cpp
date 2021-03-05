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


template<typename Pixel>
class GraphicsOutput {
public:
    GraphicsOutput() {
        gfx=get_graphics();
        mode=gfx->Mode;
        info=mode->Info;
        wid=info->HorizontalResolution;
        ht=info->VerticalResolution;
        framebuffer=(Pixel *)mode->FrameBufferBase;
    }
    
    /// Get a read-write reference to pixel (x,y).  This location must be in bounds.
    Pixel &at(int x,int y) { return framebuffer[x+y*wid]; }
    
    /// Draw a box at this cx,cy center, side sizes, and color.
    void draw_box(int cx,int cy,int sidex,int sidey,const Pixel &color)
    {
        for (int y=cy-sidey;y<cy+sidey;y++)
        for (int x=cx-sidex;x<cx+sidex;x++)
            at(x,y)=color;
    }
    
    /// Draw a circle at this cx,cy center, radius, and color.
    void draw_circle(int cx,int cy,int radius,const Pixel &color)
    {
        for (int y=-radius;y<+radius;y++)
        for (int x=-radius;x<+radius;x++)
        {
            int r2=x*x+y*y;
            if (r2<radius*radius) {
                int alpha=255-sqrtf(r2)*255/radius;
                at(cx+x,cy+y).blend(color,alpha);
            }
        }
    }
    
    /// Print background info about the graphics resolution and format
    void print_info(void) {
        print("mode "); print((int)mode->Mode); 
           print(" of "); print((int)mode->MaxMode); print("\n");
        
        print(wid); print(" pixels in X; ");
        print(ht); print(" pixels in Y\n");
        
        print(info->PixelFormat); print(" pixel format (1==BGR_)\n");
        print((uint64_t)framebuffer); print(" framebuffer base address\n");
    }

    /// It's useful to know the size of the output, so make these public:
    int wid,ht;
    Pixel *framebuffer;
    
private:    
    EFI_GRAPHICS_OUTPUT_PROTOCOL *gfx;
    EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE *mode;
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *info;
};

// Default pixel type on UEFI seems to be BGRA
struct BGRAPixel {
    UINT8 B;
    UINT8 G;
    UINT8 R;
    UINT8 A;
    
    BGRAPixel(int r,int g,int b,int a=0) {
        R=r; G=g; B=b; A=a;
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

// Draw a HAL-style glowing eye
void draw_HAL()
{
    GraphicsOutput<BGRAPixel> graphics;
    int sz=64; // radius of the disk
    int cx=graphics.wid-sz+1, cy=sz+1;
    graphics.draw_circle(cx,cy,sz,BGRAPixel(255,0,0)); // big red disk
    graphics.draw_circle(cx,cy,12,BGRAPixel(255,255,0)); // yellow middle dot
}

void print_graphics()
{
    GraphicsOutput<BGRAPixel> graphics;
    graphics.print_info();
}

void test_graphics()
{
    draw_HAL();
}






