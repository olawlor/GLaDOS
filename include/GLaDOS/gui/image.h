/**
  Group Led and Designed Operating System (GLaDOS)
  
  Image input utility code
  
  Dr. Orion Lawlor and the UAF CS 321 class, 2021-03 (Public Domain)
*/
#ifndef __GLADOS_GUI_IMAGE_H
#define __GLADOS_GUI_IMAGE_H

#ifndef __GLADOS_GUI_GRAPHICS_H
# include "graphics.h"
#endif

/** Load a 32-bit alpha PNG from this buffer of in-memory data,
   which needs to represent a PNG image of some kind. */
class PngImage : public GraphicsOutput<BGRAPixel>
{
public:
    PngImage(const void *imageData,uint64_t imageDataBytes);
    ~PngImage();
};

/// Hardcoded images baked into the kernel.
class KernelBuiltinImages {
public:
    PngImage mouse;
    PngImage courier; // font image, 16x16 pixels
    
    /// Return a reference to a single copy of the loaded images.
    ///  (Avoids global initialization problems by doing this delayed.)
    static const KernelBuiltinImages &load();
protected:
    KernelBuiltinImages(); //<- call load, don't make one yourself.
};

/// Represents a rasterized font, ready to be drawn onscreen.
class Font {
public:
    /// Load up a font by this name
    Font(const char *name="Courier",int size=14);
    
    /// Load up the default font
    static Font &load();

    /// Render this UTF-8 text to this location in this image.
    ///  Returns the new text start point (in pixels).
    Point draw(const StringSource &text,const Point &start,
            const ScreenPixel &color,GraphicsOutput<ScreenPixel> &gfx) const;

private:    
    const GraphicsOutput<BGRAPixel> &chars;
    Rect charBox;
};


#endif

