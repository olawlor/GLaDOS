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
    int sanity1;
    PngImage mouse;
    PngImage courier; // font image, 16x16 pixels
    int sanity2;
    
    /// Return a reference to a single copy of the loaded images.
    ///  (Avoids global initialization problems by doing this delayed.)
    static const KernelBuiltinImages &load();
protected:
    KernelBuiltinImages(); //<- call load, don't make one yourself.
};

#endif

