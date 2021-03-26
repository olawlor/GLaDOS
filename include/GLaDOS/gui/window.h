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
#ifndef __GLADOS_GUI_IMAGE_H
#  include "image.h" /* for PngImage */
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
        :windowTitleFont(Font::load()),
         framebuffer(framebuffer_),
         backbuffer(framebuffer.wid,framebuffer.ht)
    {
    }
    virtual ~WindowManager() {}

/// Add a Window, allocated with new.
///   We will delete the window when it's time.
    void add(Window *w) {
        windows.push(w);
    }

/// Font for window titles
    const Font &windowTitleFont;

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
        // Bring in a normal arrow shape
        const PngImage &cursor=KernelBuiltinImages::load().mouse;
        
        int x=mouse.x-2; // shift to cursor hotspot
        int y=mouse.y-3;
        Rect cursorRect=cursor.frame.shifted(x,y);
        
        cursor.blendTo(cursor.frame,cursorRect,gfx);
        
        // draw cursor in black:
        //backbuffer.fillRect(cursorRect,0);
        
        // white glowy tip
        //backbuffer.drawBlendCircle(mouse.x,mouse.y,3,0xffffff);
    }

    struct WindowColors {
        ScreenPixel titlebar; // behind text on the titlebar
        ScreenPixel title; // text in window title
        ScreenPixel border; // hard border around window
    };
    WindowColors topColors={ 0x685556, 0xffffff, 0xff0000 };
    WindowColors backColors={ 0x483536, 0xffffff, 0xff0000 };


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
        
        windowTitleFont.draw(w.name,title.topleft()+Point(8,20),
            colors.title,gfx);
    }
    
    /// Draw window decorations (shadow, titlebar, etc) for this window
    virtual void drawWindowDecorations(GraphicsOutput<ScreenPixel> &gfx,bool isTop,Window &w)
    {
        drawWindowDecorationsColor(gfx,
            isTop?topColors:backColors,
            w);
    }
    
    /// Draw all onscreen windows back to front
    virtual void drawAllWindows(GraphicsOutput<ScreenPixel> &gfx)
    {
        // Draw windows back-to-front (currently in wrong stack order)
        vector<Window *> reorder;
        for (Window &w:windows) reorder.push_back(&w);
        for (int i=reorder.size()-1;i>=0;i--)
        {
            Window &w=*reorder[i];
	        drawWindowDecorations(gfx,i==0,w);
            w.draw(gfx);
        }
    }

    /// Draw everything onscreen: desktop, windows, mouse.
    virtual void drawScreen(GraphicsOutput<ScreenPixel> &framebuffer)
    {
        drawDesktop(backbuffer);
        
        drawAllWindows(backbuffer);
        
        drawMouse(backbuffer);
        
        backbuffer.copyTo(backbuffer.frame,framebuffer.frame,framebuffer);
    }
    
/// Event handling:
    MouseState mouse;
    
    Window *keyboardFocus=0;

/// Preferences area of window manager
    typedef enum { 
        clickFocus=1, // click mouse in window (or alt-tab), it has keyboard focus
        mouseFocus=2, // mouse over window, it has keyboard focus
     } focusMode_t;
    focusMode_t focusMode=clickFocus; 
    
    virtual void handleKeystroke(const KeyTyped &key) {
        // Route to the focused window first
        if (focusMode==clickFocus && keyboardFocus && keyboardFocus->handler) {
            keyboardFocus->handler->handleKeystroke(key);
            return;
        }
    
        // Route keystroke to the window the mouse is currently over
        for (Window &w:windows) 
            if (w.onscreen.contains(mouse) && w.handler)
            {
                w.handler->handleKeystroke(key);
                return;
            }
        
        // Keystroke out in the desktop: reset the desktop color (demo)
        if (key.unicode=='r') desktopColor=0xff0000; // turn desktop red
        if (key.unicode=='g') desktopColor=0x808080; // turn desktop gray again
    }
    
    virtual void handleMouse(MouseState &newMouse) {
        // Clip the mouse position to lie onscreen
        if (newMouse.x<0) newMouse.x=0;
        if (newMouse.y<0) newMouse.y=0;
        if (newMouse.x>=framebuffer.wid) newMouse.x=framebuffer.wid-1;
        if (newMouse.y>=framebuffer.ht)  newMouse.y=framebuffer.ht-1;
        
        // Update the mouse position
        MouseState oldMouse=mouse;
        mouse=newMouse;
        
        // Route the event to the appropriate window:
        for (Window &w:windows) 
        {
            // Check for mouse interaction with the titlebar:
            //   Subtle: check against oldMouse, so we can't outrun window drag.
            if (mouse.buttonDown(0) && windowToTitlebar(w).contains(oldMouse))
            {
                // FIXME: check for widgets like close
                
                // Drag the window around
                w.move(mouse-oldMouse);
                return;
            }
            
            // Check for interaction with a window content area
            //   FIXME: do we want enter/leave events too?
            if (w.onscreen.contains(mouse))
            {
                if (&w != keyboardFocus && focusMode==clickFocus && mouse.buttonDown(0))
                { // update keyboard focus (click to focus)
                    keyboardFocus=&w;
                    return; //<- click doesn't get passed to application
                }
                
                if (w.handler)
                {
                    // Shift to local coordinates:
                    MouseState local=mouse;
                    local.makeOrigin(w.onscreen.topleft());
                    w.handler->handleMouse(local);
                    return;
                }
            }
        }
        
        // If we get here, the mouse is on the desktop?
        if (focusMode==clickFocus && mouse.buttonDown(0))
            keyboardFocus=0;
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

