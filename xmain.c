#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <err.h>

#include <cairo.h>
#include <freetype2/ft2build.h>
#include FT_FREETYPE_H
#include <utf8proc.h>

#include "mace.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/XKBlib.h>
#include <X11/keysymdef.h>
#include <cairo-xlib.h>

static Display *display;
static Window win;
static int screen;

static cairo_surface_t *sfc;

static void
xresize(int w, int h)
{
  width = w;
  height = h;

  if (!tabresize(tab, 0, 0, w, h)) {
    errx(1, "Failed to resize panes!");
  }

  cairo_xlib_surface_set_size(sfc, w, h);

  cairo_push_group(cr);

  tabdraw(tab);

  cairo_pop_group_to_source(cr);
  cairo_paint(cr);
  cairo_surface_flush(sfc);
}

static keycode_t
symtocode(KeySym sym)
{
  switch (sym) {
  case XK_Shift_L:
  case XK_Shift_R:
    return KEY_shift;
    
  case XK_Alt_L:
  case XK_Alt_R:
  case XK_Meta_L:
  case XK_Meta_R:
    return KEY_alt;
    
  case XK_Super_L:
  case XK_Super_R:
    return KEY_super;

  case XK_Control_L:
  case XK_Control_R:
    return KEY_control;

  case XK_Left: return KEY_left;
  case XK_Right: return KEY_right;
  case XK_Up: return KEY_up;
  case XK_Down: return KEY_down;
  case XK_Page_Up: return KEY_pageup;
  case XK_Page_Down: return KEY_pagedown;
  case XK_Home: return KEY_home;
  case XK_End: return KEY_end;

  case XK_Return: return KEY_return;
  case XK_Tab: return KEY_tab;
  case XK_BackSpace: return KEY_backspace;
  case XK_Delete: return KEY_delete;
  case XK_Escape: return KEY_escape;

  default:
    return KEY_none;
  }
}

static void
xhandlekeypress(XKeyEvent *e)
{
  uint8_t s[16];
  keycode_t c;
  KeySym sym;
  ssize_t n;

  sym = XkbKeycodeToKeysym(display, e->keycode, 0,
			   e->state & ShiftMask);

  c = symtocode(sym);

  cairo_push_group(cr);

  if (c == KEY_none) {
    n = utf8proc_encode_char(sym, s);
    if (n > 0) {
      handletyping(s, n);
    }
  } else {
    handlekeypress(c);
  }

  cairo_pop_group_to_source(cr);
  cairo_paint(cr);
  cairo_surface_flush(sfc);
}

static void
xhandlekeyrelease(XKeyEvent *e)
{
  keycode_t c;
  KeySym sym;

  sym = XkbKeycodeToKeysym(display, e->keycode, 0,
			   e->state & ShiftMask);

  c = symtocode(sym);

  if (c != KEY_none) {
    cairo_push_group(cr);
    
    handlekeyrelease(c);

    cairo_pop_group_to_source(cr);
    cairo_paint(cr);
    cairo_surface_flush(sfc);
  }
}

static void
xhandlebuttonpress(XButtonEvent *e)
{
  cairo_push_group(cr);

  switch (e->button) {
  case 1:
  case 2:
  case 3:
    handlebuttonpress(e->x, e->y, e->button);
    break;

  case 4:
    handlescroll(e->x, e->y, -5);
    break;
    
  case 5:
    handlescroll(e->x, e->y, 5);
    break;
  }

  cairo_pop_group_to_source(cr);
  cairo_paint(cr);
  cairo_surface_flush(sfc);
}

static void
xhandlebuttonrelease(XButtonEvent *e)
{
  cairo_push_group(cr);

  switch (e->button) {
  case 1:
  case 2:
  case 3:
    handlebuttonrelease(e->x, e->y, e->button);
    break;
  }

  cairo_pop_group_to_source(cr);
  cairo_paint(cr);
  cairo_surface_flush(sfc);
}

static void
xhandlemotion(XMotionEvent *e)
{
  cairo_push_group(cr);

  handlemotion(e->x, e->y);

  cairo_pop_group_to_source(cr);
  cairo_paint(cr);
  cairo_surface_flush(sfc);
} 

static void
eventLoop(void)
{
  XEvent e;

  while (true) {
    XNextEvent(display, &e);

    switch (e.type) {

    case ConfigureNotify:
      if (e.xconfigure.width != width
	  || e.xconfigure.height != height) {

	xresize(e.xconfigure.width, e.xconfigure.height);
      }
      
      break;

    case Expose:
      cairo_push_group(cr);

      tabdraw(tab);

      cairo_pop_group_to_source(cr);
      cairo_paint(cr);
      cairo_surface_flush(sfc);
      break;

    case KeyPress:
      xhandlekeypress(&e.xkey);
      break;

    case KeyRelease:
      xhandlekeyrelease(&e.xkey);
      break;

    case ButtonPress:
      xhandlebuttonpress(&e.xbutton);
      break;

    case ButtonRelease:
      xhandlebuttonrelease(&e.xbutton);
      break;

    case MotionNotify:
      xhandlemotion(&e.xmotion);     
      break;

    default:
      printf("unknown event %i\n", e.type);
    }
  }
}

int
main(int argc, char **argv)
{
  width = 800;
  height = 500;
  
  display = XOpenDisplay(NULL);
  if (display == NULL) {
    err(1, "Failed to open X display!");
  }
  
  screen = DefaultScreen(display);

  win = XCreateSimpleWindow(display, RootWindow(display, screen),
			    0, 0, width, height, 5, 0, 0);

  XSelectInput(display, win, ExposureMask | StructureNotifyMask
	       | KeyPressMask | KeyReleaseMask | PointerMotionMask
	       | ButtonPressMask | ButtonReleaseMask);

  XSetStandardProperties(display, win, "Mace", "Mace",
			 None, NULL, 0, NULL);

  XMapWindow(display, win);

  sfc = cairo_xlib_surface_create(display, win,
				  DefaultVisual(display, screen),
				  width, height);

  cr = cairo_create(sfc);

  init();
  fontinit();

  tabresize(tab, 0, 0, width, height);

  cairo_push_group(cr);

  tabdraw(tab);

  cairo_pop_group_to_source(cr);
  cairo_paint(cr);
  cairo_surface_flush(sfc);

  eventLoop();

  cairo_destroy(cr);
  cairo_surface_destroy(sfc);
  
  XDestroyWindow(display, win);
  XCloseDisplay(display);

  return 0;
}
