/**
  Group Led and Designed Operating System (GLaDOS)
  
  Draw things in windows, with title bars, and event handling.
  
  Dr. Orion Lawlor and the UAF CS 321 class, 2021-03 (Public Domain)
*/
#ifndef __GLADOS_GUI_WINDOW_H
#define __GLADOS_GUI_WINDOW_H

#ifndef __GLADOS_GUI_GRAPHICS_H
#  include "graphics.h" /* for "Point" definition */
#endif

#ifndef __GLADOS_GUI_EVENT_H
#  include "event.h" /* for "UserEventHandler" */
#endif


/// A Window is a box visible onscreen.
class Window {
public:
    // Part of the IntrusiveList interface:
    Window *next;
    
    // Destination for window events (or 0 if none)
    UserEventHandler *handler;

    const char *name; // human-readable name string
	Rect onscreen; // location and dimensions of window onscreen
	OffscreenGraphics<ScreenPixel> offscreen; // window's contents
    
    Window(const char *name_,const Rect &onscreen_)
        :next(0),
         handler(0),
         name(name_),
         onscreen(onscreen_),
         offscreen(onscreen.X.size(),onscreen.Y.size())
    {}
    
    virtual ~Window() {}
    
    /// Move this window onscreen
    virtual void move(const Point &distance) {
        onscreen=onscreen.shifted(distance);
    }
    
	/// Draw method: update window display onscreen
	virtual void draw(GraphicsOutput<ScreenPixel> &gfx) {
	    offscreen.copyTo(offscreen.frame,onscreen,gfx); // contents
	}
};


/// Draws stuff onscreen as events happen!
///   In charge of the back buffer, window decorations, and compositing.
class WindowManager : public UserEventHandler {
public:
/// Make a WindowManager to handle this framebuffer
    WindowManager(GraphicsOutput<ScreenPixel> &framebuffer_)
        :framebuffer(framebuffer_),
         backbuffer(framebuffer.wid,framebuffer.ht)
    {
    }
    virtual ~WindowManager() {}

/// Add a Window, allocated with new.
///   We will delete the window when it's time.
    void add(Window *w) {
        windows.push(w);
    }

/// Screen draw
    int desktopColor=0x808080;
    
    /// Desktop background (virtual so folks can override this)
    virtual void drawDesktop(GraphicsOutput<ScreenPixel> &gfx)
    {
        // Background of desktop
        gfx.fillRect(gfx.frame,desktopColor);
        
        // glowy eye of GLaDOS
        draw_HAL(gfx);
    }
    /// Draw a HAL-style glowing eye
    void draw_HAL(GraphicsOutput<ScreenPixel> &gfx)
    {
        int sz=64; // radius of the disk
        int cx=gfx.wid-sz+1, cy=sz+1;
        gfx.drawBlendCircle(cx,cy,sz,ScreenPixel(255,0,0)); // big red disk
        gfx.drawBlendCircle(cx,cy,12,ScreenPixel(255,255,0)); // yellow middle dot
    }
    
    
    /// Mouse pointer
    virtual void drawMouse(GraphicsOutput<ScreenPixel> &gfx)
    {
        int cursorSizeX=16; // pixels onscreen
        int cursorSizeY=20;
        Rect cursorRect(
            mouse.x,mouse.x+cursorSizeX,
            mouse.y,mouse.y+cursorSizeY
        );
        // FIXME: bring in a normal arrow shape here
        // draw cursor in black:
        backbuffer.fillRect(cursorRect,0);
        // white glowy tip
        backbuffer.drawBlendCircle(mouse.x,mouse.y,3,0xffffff);
    }

    struct WindowColors {
        ScreenPixel titlebar; // behind text on the titlebar
        ScreenPixel border; // hard border around window
    };
    WindowColors topColors={ 0x685556, 0xff0000 };
    WindowColors backColors={ 0x483536, 0xff0000 };


    /// Given a rect for a window, return the rect for the titlebar
    virtual Rect windowToTitlebar(const Window &win) 
    {
        const Rect &w=win.onscreen;
        return Rect(w.X.middle(),w.X.hi,
            w.Y.lo-32, w.Y.lo);
    }
    
    /// Draw window decorations for a window of this size
    virtual void drawWindowDecorationsColor(GraphicsOutput<ScreenPixel> &gfx,const WindowColors &colors,Window &w)
    {
        const Rect &r=w.onscreen;
        gfx.shadowRect(r); // content area of window
        gfx.outlineRect(r,1,colors.border);
        
        Rect title=windowToTitlebar(w);
        gfx.shadowRect(title);        
        gfx.fillRect(title,colors.titlebar);
    }
    
    /// Draw window decorations (shadow, titlebar, etc) for this window
    virtual void drawWindowDecorations(GraphicsOutput<ScreenPixel> &gfx,bool isTop,Window &w)
    {
        drawWindowDecorationsColor(gfx,
            isTop?topColors:backColors,
            w);
    }

    /// Draw everything onscreen: desktop, windows, mouse.
    virtual void drawScreen(GraphicsOutput<ScreenPixel> &framebuffer)
    {
        drawDesktop(backbuffer);
        
        // Draw windows back-to-front (currently in wrong stack order)
        vector<Window *> reorder;
        for (Window &w:windows) reorder.push_back(&w);
        for (int i=reorder.size()-1;i>=0;i--)
        {
            Window &w=*reorder[i];
	        drawWindowDecorations(backbuffer,i==0,w);
            w.draw(backbuffer);
        }
        
        drawMouse(backbuffer);
        
        backbuffer.copyTo(backbuffer.frame,framebuffer.frame,framebuffer);
    }
    
/// Event handling:
    MouseState mouse;
    
    virtual void handleKeystroke(const KeyTyped &key) {
        // Route keystroke to the window the mouse is currently over
        for (Window &w:windows) 
            if (w.onscreen.contains(mouse) && w.handler)
            {
                w.handler->handleKeystroke(key);
                return;
            }
        
        // Keystroke out in the desktop: reset the desktop color (demo)
        desktopColor=0xff0000;
    }
    
    virtual void handleMouse(MouseState &newMouse) {
        MouseState oldMouse=mouse;
        
        // Clip the mouse position to lie onscreen
        if (newMouse.x<0) newMouse.x=0;
        if (newMouse.y<0) newMouse.y=0;
        if (newMouse.x>framebuffer.wid) newMouse.x=framebuffer.wid;
        if (newMouse.y>framebuffer.ht)  newMouse.y=framebuffer.ht;
        
        mouse=newMouse;
        
        for (Window &w:windows) 
        {
            // Check for mouse interaction with the titlebar:
            //   Subtle: check against oldMouse, so we can't outrun window drag.
            if (mouse.buttonDown(0) && windowToTitlebar(w).contains(oldMouse))
            {
                // FIXME: check for widgets like close
                // drag the window around
                w.move(newMouse-oldMouse);
                return;
            }
            
            // Check for interaction with a window content area
            //   FIXME: do we want enter/leave events too?
            if (w.onscreen.contains(newMouse) && w.handler)
            {
                // Shift to local coordinates:
                MouseState local=newMouse;
                local.makeOrigin(w.onscreen.topleft());
                w.handler->handleMouse(local);
                return;
            }
        }
    }

private:
/// This is where we draw the screen
    GraphicsOutput<ScreenPixel> &framebuffer;

/// This is our offscreen buffer, used to get stuff onscreen without flicker.
    OffscreenGraphics<ScreenPixel> backbuffer;

/// This is the list of all the windows currently onscreen,
///   sorted by Z order, topmost first.
    IntrusiveList<Window> windows;
}; 




#endif

