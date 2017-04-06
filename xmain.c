#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <err.h>
#include <freetype2/ft2build.h>
#include FT_FREETYPE_H
#include <utf8proc.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/XKBlib.h>
#include <X11/keysymdef.h>

#include "mace.h"

static Display *display;
static Window win;
static int screen;
static XImage *img = NULL;

static void
xresize(int w, int h)
{
  unsigned char *nbuf;

  nbuf = malloc(w * h * 4);
  if (nbuf == NULL) {
    err(1, "Failed to allocate window buffer!\n");
  }

  buf = nbuf;
  width = w;
  height = h;

  paneresize(root, 0, 0, w, h);
  panedraw(root);

  if (img != NULL) {
    XDestroyImage(img);
  }

  img = XCreateImage(display, CopyFromParent, 24, ZPixmap, 0,
		     (char *) buf,
		     width, height, 32, 0);
}

static void
updatewindow(void)
{
  XPutImage(display, win, DefaultGC(display, screen), img,
	    0, 0, 0, 0, width, height);
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
  bool redraw;
  KeySym sym;
  ssize_t n;

  sym = XkbKeycodeToKeysym(display, e->keycode, 0,
			   e->state & ShiftMask);

  c = symtocode(sym);

  if (c == KEY_none) {
    n = utf8proc_encode_char(sym, s);
    if (n > 0) {
      redraw = handletyping(s, n);
    } else {
      redraw = false;
    }
  } else {
    redraw = handlekeypress(c);
  }

  if (redraw) {
    updatewindow();
  }
}

static void
xhandlekeyrelease(XKeyEvent *e)
{
  keycode_t c;
  bool redraw;
  KeySym sym;

  sym = XkbKeycodeToKeysym(display, e->keycode, 0,
			   e->state & ShiftMask);

  c = symtocode(sym);

  if (c != KEY_none) {
    redraw = handlekeyrelease(c);
  } else {
    redraw = false;
  }

  if (redraw) {
    updatewindow();
  }
}

static void
xhandlebuttonpress(XButtonEvent *e)
{
  bool redraw;

  switch (e->button) {
  case 1:
  case 2:
  case 3:
    redraw = handlebuttonpress(e->x, e->y, e->button);
    break;

  case 4:
    redraw = handlescroll(e->x, e->y, 0, -lineheight / 2);
    break;
    
  case 5:
    redraw = handlescroll(e->x, e->y, 0, lineheight / 2);
    break;

  default:
    redraw = false;
  }
  
  if (redraw) {
    updatewindow();
  }
}

static void
xhandlebuttonrelease(XButtonEvent *e)
{
  bool redraw;

  switch (e->button) {
  case 1:
  case 2:
  case 3:
    redraw = handlebuttonrelease(e->x, e->y, e->button);
    break;

  default:
    redraw = false;
  }
  
  if (redraw) {
    updatewindow();
  }
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

      updatewindow();
      break;

    case Expose:
      updatewindow();
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
      if (handlemotion(e.xmotion.x, e.xmotion.y)) {
	updatewindow();
      }
      
      break;

    default:
      printf("unknown event %i\n", e.type);
    }
  }
}

int
main(int argc, char **argv)
{
  int width, height;
  
  width = 800;
  height = 500;
  
  display = XOpenDisplay(NULL);
  if (display == NULL) {
    err(1, "Failed to open X display!\n");
  }
  
  screen = DefaultScreen(display);

  win = XCreateSimpleWindow(display, RootWindow(display, screen),
			    0, 0, width, height, 5, 0, 0);

  XSelectInput(display, win, ExposureMask | StructureNotifyMask
	       | KeyPressMask | KeyReleaseMask | PointerMotionMask
	       | ButtonPressMask | ButtonReleaseMask);

  XSetStandardProperties(display, win, "mace", "um",
			 None, NULL, 0, NULL);

  init();

  xresize(width, height);
  
  XMapWindow(display, win);
  
  eventLoop();

  XDestroyWindow(display, win);
  XCloseDisplay(display);

  return 0;
}
