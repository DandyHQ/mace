#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>
#include <err.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/XKBlib.h>
#include <X11/keysymdef.h>
#include <cairo-xlib.h>

#include "mace.h"
#include "config.h"

#include "resources/xlib-keysyms.h"

static Display *display;
static Window win;
static int screen;

static int width, height;

static cairo_surface_t *sfc;
static cairo_t *cr;

static struct mace *mace;

static void
xresize(int w, int h)
{
  width = w;
  height = h;

  cairo_xlib_surface_set_size(sfc, w, h);

  if (!paneresize(mace->pane, 0, 0, w, h)) {
    /* What should actually happen here? */
    errx(1, "Failed to resize!");
  }
}

static int32_t
symtounicode(KeySym sym)
{
	int32_t code = 0;
	int i;

	/* Fuck Xorg */

	/* Keysyms under 0x100 are ASCII so unicode. */
	if (0x20 <= sym && sym < 0x100) {
		code = sym;

	/* At this point unicode was around and people decieded to 
	   just shift the values. */
	} else if (0x01000100 <= sym && sym <= 0x0110ffff) {
		code = sym - 0x01000000;

	/* But before that weird things happened. */
	} else if (0x0100 <= sym && sym <= 0x20ff) {
		
		for (i = 0; i < sizeof(keymappings) / sizeof(keymappings[0]); i++) {
			if (keymappings[i].keysym == sym) {
				code = keymappings[i].unicode;
				break;
			}
		}
	}

	return code;
}

static size_t
encodekey(KeySym sym, uint8_t *s, size_t n)
{
	int32_t code;

  switch (sym) {
  case XK_Shift_L:
  case XK_Shift_R:
  case XK_Alt_L:
  case XK_Alt_R:
  case XK_Meta_L:
  case XK_Meta_R:
  case XK_Super_L:
  case XK_Super_R:
  case XK_Control_L:
  case XK_Control_R:
    return 0;

  case XK_Left:      strcpy((char *) s, "Left"); break;
  case XK_Right:     strcpy((char *) s, "Right"); break;
  case XK_Up:        strcpy((char *) s, "Up"); break;
  case XK_Down:      strcpy((char *) s, "Down"); break;
  case XK_Page_Up:   strcpy((char *) s, "PageUp"); break;
  case XK_Page_Down: strcpy((char *) s, "PageDown"); break;
  case XK_Home:      strcpy((char *) s, "Home"); break;
  case XK_End:       strcpy((char *) s, "End"); break;
  case XK_Return:    strcpy((char *) s, "Return"); break;
  case XK_Tab:       strcpy((char *) s, "Tab"); break;
	case XK_BackSpace: strcpy((char *) s, "BackSpace"); break;
  case XK_Delete:    strcpy((char *) s, "Delete"); break;
  case XK_Escape:    strcpy((char *) s, "Escape"); break;

  default:
    code = symtounicode(sym);
		if (code == 0) {
			return 0;
		}

		return utf8encode(s, n, code);
  }

	return strlen((char *) s);
}

static bool
xhandlekeypress(XKeyEvent *e)
{
  size_t n = 32, nn = 0;
  KeySym sym;
  uint8_t s[n];

  sym = XkbKeycodeToKeysym(display, e->keycode, 0,
			   e->state & ShiftMask);

	if ((e->state & ShiftMask) != 0) {
		memmove(s + nn, "S-", 2);
		nn += 2;
	}

	if ((e->state & ControlMask) != 0) {
		memmove(s + nn, "C-", 2);
		nn += 2;
	}

	if ((e->state & Mod1Mask) != 0) {
		memmove(s + nn, "A-", 2);
		nn += 2;;
	}

	if ((e->state & Mod4Mask) != 0) {
		memmove(s + nn, "M-", 2);
		nn += 2;
	}

  n = encodekey(sym, s + nn, n - nn);
  if (n > 0) {
    return handlekey(mace, s, n + nn);
  } else {
		return false;
	}
}

static bool
xhandlebuttonpress(XButtonEvent *e)
{
  switch (e->button) {
  case 1:
  case 2:
  case 3:
    return handlebuttonpress(mace, e->x, e->y, e->button);

  case 4:
    return handlescroll(mace, e->x, e->y, 0, -mace->font->lineheight);
    
  case 5:
    return handlescroll(mace, e->x, e->y, 0, mace->font->lineheight);

  default:
    return false;
  }
}

static bool
xhandlebuttonrelease(XButtonEvent *e)
{
  switch (e->button) {
  case 1:
  case 2:
  case 3:
    return handlebuttonrelease(mace, e->x, e->y, e->button);
  default:
    return false;
  }
}

static bool
xhandlemotion(XMotionEvent *e)
{
  return handlemotion(mace, e->x, e->y);
} 

static void
eventLoop(void)
{
  bool redraw;
  XEvent e;

  while (mace->running) {
    XNextEvent(display, &e);

    redraw = false;
    
    switch (e.type) {

    case ConfigureNotify:
      if (e.xconfigure.width != width
			    || e.xconfigure.height != height) {

				xresize(e.xconfigure.width, e.xconfigure.height);
				redraw = true;
      }
      
      break;

    case Expose:
      redraw = true;
     break;

    case KeyPress:
      redraw = xhandlekeypress(&e.xkey);
      break;

    case ButtonPress:
      redraw = xhandlebuttonpress(&e.xbutton);
      break;

    case ButtonRelease:
      redraw = xhandlebuttonrelease(&e.xbutton);
      break;

    case MotionNotify:
      redraw = xhandlemotion(&e.xmotion);     
      break;
    }

    if (redraw) {
      cairo_push_group(cr);

      panedraw(mace->pane, cr);

      cairo_pop_group_to_source(cr);
      cairo_paint(cr);
      cairo_surface_flush(sfc);
    }
  }
}

int
main(int argc, char **argv)
{
  int width, height, i;
	struct tab *t;

  width = 800;
  height = 500;
  
	mace = macenew();
  if (mace == NULL) {
    errx(1, "Failed to initalize mace!");
  }

	for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-v") == 0) {
			printf ("This is Mace version %s\n", VERSION_STR);
			macefree(mace);
			return 0;

		} else {
			t = tabnewfromfile(mace, (uint8_t *) argv[i], strlen(argv[i]));
			if (t == NULL) {
				continue;
			}
			paneaddtab(mace->pane, t, -1);
			mace->pane->focus = t;
			mace->mousefocus = t->main;
		}	
	}

	/* This is ugly. Removes default tab is files were opened. */
	if (mace->pane->focus != mace->pane->tabs) {
		t = mace->pane->tabs;
		paneremovetab(mace->pane, t);
		tabfree(t);
	}


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

  xresize(width, height);

  cairo_push_group(cr);

  panedraw(mace->pane, cr);

  cairo_pop_group_to_source(cr);
  cairo_paint(cr);
  cairo_surface_flush(sfc);

  eventLoop();

  macefree(mace);
  
  cairo_destroy(cr);
  cairo_surface_destroy(sfc);
  
  XDestroyWindow(display, win);
  XCloseDisplay(display);

  return 0;
}
