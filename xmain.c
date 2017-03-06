#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/XKBlib.h>
#include <X11/keysymdef.h>
#include <err.h>
#include <freetype2/ft2build.h>
#include FT_FREETYPE_H

#include "mace.h"

Display *display;
Window win;
int screen;
XImage *img = NULL;

static void
xresize(int w, int h)
{
  char *nbuf;

  nbuf = malloc(w * h * 4);
  if (nbuf == NULL) {
    err(1, "Failed to allocate window buffer!\n");
  }

  resize(nbuf, w, h);

  if (img != NULL) {
    XDestroyImage(img);
  }

  img = XCreateImage(display, CopyFromParent, 24, ZPixmap, 0, buf,
		     width, height, 32, 0);
}

static void
updatewindow(void)
{
  XPutImage(display, win, DefaultGC(display, screen), img,
	    0, 0, 0, 0, width, height);
}

static void
xhandlekeypress(XKeyEvent *e)
{
  KeySym sym;

  sym = XkbKeycodeToKeysym(display, e->keycode, 0,
			   e->state & ShiftMask);

  if (handlekeypress((unsigned int) sym)) {
    updatewindow();
  }
}

static void
xhandlekeyrelease(XKeyEvent *e)
{
  KeySym sym;

  sym = XkbKeycodeToKeysym(display, e->keycode, 0,
			   e->state & ShiftMask);

  if (handlekeyrelease((unsigned int) sym)) {
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
      if (handlebuttonpress(e.xbutton.x, e.xbutton.y,
			    e.xbutton.button)) {
	updatewindow();
      }
      
      break;

    case ButtonRelease:
      if (handlebuttonrelease(e.xbutton.x, e.xbutton.y,
			      e.xbutton.button)) {
	updatewindow();
      }
      
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
