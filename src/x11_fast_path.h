// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

// X11 fast path for the scene renderer.
//
// The authored scene pass contains many small primitives.  Xlib is asynchronous,
// but repeatedly sending identical GC state and redrawing once for every queued
// pointer-motion event can still swamp the client/server connection and make the
// UI feel several clicks behind.  This header is force-included for the X11
// executable so we can keep the renderer simple while avoiding those costs.

#include <X11/Xlib.h>

namespace backyard_racer::x11_fast_path {

inline int set_foreground(Display* display, GC gc, unsigned long pixel) {
    // Backyard Racer uses one GC.  Keep the GC in the cache key anyway so this
    // remains correct if another GC is introduced later.
    static GC cached_gc = nullptr;
    static unsigned long cached_pixel = 0;
    static bool valid = false;
    if (valid && gc == cached_gc && pixel == cached_pixel) return 1;
    cached_gc = gc;
    cached_pixel = pixel;
    valid = true;
    return ::XSetForeground(display, gc, pixel);
}

inline int set_line_attributes(Display* display, GC gc, unsigned int width,
                               int line_style, int cap_style, int join_style) {
    static GC cached_gc = nullptr;
    static unsigned int cached_width = 0;
    static int cached_line_style = LineSolid;
    static int cached_cap_style = CapButt;
    static int cached_join_style = JoinMiter;
    static bool valid = false;

    if (valid && gc == cached_gc && width == cached_width &&
        line_style == cached_line_style && cap_style == cached_cap_style &&
        join_style == cached_join_style)
        return 1;

    cached_gc = gc;
    cached_width = width;
    cached_line_style = line_style;
    cached_cap_style = cap_style;
    cached_join_style = join_style;
    valid = true;
    return ::XSetLineAttributes(display, gc, width, line_style, cap_style, join_style);
}

inline int next_event(Display* display, XEvent* event) {
    const int result = ::XNextEvent(display, event);

    // Keep only the newest event from high-volume visual-only streams.  Button
    // and keyboard events stay in the queue in order and are never discarded.
    if (event->type == MotionNotify || event->type == ConfigureNotify ||
        event->type == Expose) {
        XEvent newer{};
        const int type = event->type;
        const Window window = event->xany.window;
        while (::XCheckTypedWindowEvent(display, window, type, &newer))
            *event = newer;
    }

    return result;
}

} // namespace backyard_racer::x11_fast_path

// These macros affect only calls appearing after this force-included header.
// The wrappers above were compiled against the real Xlib functions.
#define XSetForeground backyard_racer::x11_fast_path::set_foreground
#define XSetLineAttributes backyard_racer::x11_fast_path::set_line_attributes
#define XNextEvent backyard_racer::x11_fast_path::next_event
