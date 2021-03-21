/**
  Group Led and Designed Operating System (GLaDOS)
  
  Graphical output utilities, for drawing things on or off screen
  
  Dr. Orion Lawlor and the UAF CS 321 class, 2021-03 (Public Domain)
*/
#ifndef __GLADOS_GUI_GRAPHICS_H
#define __GLADOS_GUI_GRAPHICS_H

/// Default pixel type on UEFI seems to be BGRA:
///  This class stores one 32-bit pixel.
struct BGRAPixel {
    UINT8 B;
    UINT8 G;
    UINT8 R;
    UINT8 A;
    
    /// Initialize to black
    BGRAPixel() { B=G=R=A=0; }
    
    /// Initialize from 0-255 byte values.
    BGRAPixel(int r,int g,int b,int a=0) {
        R=r; G=g; B=b; A=a;
    }
    
    /// Initialize from HTML-style colors: 0xRRGGBB
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



/**
 A GraphicsOutput is a place to draw things:
    - The framebuffer
    - An offscreen buffer like the back buffer
*/
template<typename Pixel>
class GraphicsOutput {
public:
    // It's useful to know the size of the output, so make these public:

    /// Size of the framebuffer, in pixels
    int wid,ht; 
    /// Distance from one row to the next row
    int pixelsPerRow;
    /// Image dimensions as a Rect, for clipping
    Rect frame;
    /// This is where we draw our pixels
    Pixel *framebuffer;
    
    /// Create a new 
    GraphicsOutput(int wid_,int ht_,int pixelsPerRow_,Pixel *framebuffer_)
        :wid(wid_), ht(ht_), pixelsPerRow(pixelsPerRow_), 
         frame(0,wid,0,ht),
         framebuffer(framebuffer_)
    {}
    
    /// Get a read-write reference to pixel (x,y).  This location must be in bounds.
    Pixel &at(int x,int y) { return framebuffer[x+y*pixelsPerRow]; }
    
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
    
    /// Draw a dark shadow underneath this rectangle
    void shadowRect(const Rect &r)
    {
        Rect shadow=r.shift(2,2);
        // Right side:
        fillRect(Rect(r.X.hi,shadow.X.hi,shadow.Y.lo,shadow.Y.hi),0); 
        // Bottom side:
        fillRect(Rect(shadow.X.lo,shadow.X.hi,r.Y.hi,shadow.Y.hi),0); 
    }
    
    /// Draw a blended circle at this cx,cy center, radius, and color.
    ///   We slowly get more like color as we approach the middle.
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
        :GraphicsOutput<Pixel>(wid_,ht_,wid_,new Pixel[wid_*ht_])
    {}
    ~OffscreenGraphics() {
        delete[] this->framebuffer; this->framebuffer=0;
    }
};


#endif

