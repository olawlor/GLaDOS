/**
  Group Led and Designed Operating System (GLaDOS)
  
  Event handling: keyboard and mouse input.
  
  Dr. Orion Lawlor and the UAF CS 321 class, 2021-03 (Public Domain)
*/
#ifndef __GLADOS_GUI_EVENT_H
#define __GLADOS_GUI_EVENT_H

#ifndef __GLADOS_GUI_GRAPHICS_H
#  include "graphics.h" /* for "Point" definition */
#endif

/// Key typed notification.
///   This is for text input, meaning keyup and key repeat, not down/up events.
struct KeyTyped {
    /// Keyboard scan code
    int scancode;
    
    /// Unicode character code, including ASCII codes
    int unicode;
    
    /// State of modifier keys
    int modifiers;
    
    KeyTyped() { scancode=unicode=modifiers=0; }
};

/// Abstract mouse movement notification.
///  The mouse is at this onscreen location.
struct MouseState : public Point {
    /// Fields x and y are inherited from Point

    /// Bit mask of pressed buttons:
    ///   bit 0 is left mouse button
    ///   bit 1 is right mouse button
    ///   bit 2 is scroll wheel
    int buttons;
    
    /// Return true if mouse button i is pressed.
    bool buttonDown(int i) const {
        return buttons & (1<<i); 
    }
    
    /// State of scroll wheel
    int scroll;
    
    /// State of modifier keys
    int modifiers;
    
    MouseState() { buttons=scroll=modifiers=0; }
    
    // Shift this mouse position so p is the new origin
    void makeOrigin(const Point &p) {
        x-=p.x;
        y-=p.y;
    }
};


/// Abstract target for user interface events.
///  Inherit from this class to handle new event types.
class UserEventHandler {
public:
    virtual void handleKeystroke(const KeyTyped &key) {}
    virtual void handleMouse(MouseState &mouse) {}
}; 

/// Watches for UEFI events, and passes them to a handler.
class UserEventSource {
public:    
    UserEventSource();
    
    /// Wait up to delay milliseconds for a user event.
    ///  If one arrives, pass it to this handler and return true.
    ///  If nothing happens, return false.
    bool waitForEvent(int delayMS,UserEventHandler &handler);

private:
    bool probeMouse(void);
    MouseState mouse;
};


#endif



