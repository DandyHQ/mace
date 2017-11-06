#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>
#include <err.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/XKBlib.h>
#include <X11/keysymdef.h>
#include <X11/Xatom.h>
#include <cairo-xlib.h>

#include "mace.h"

#include "resources/xlib-keysyms.h"

static Display *display;
static Window win;
static int screen;

static int width, height;

static cairo_surface_t *sfc;
static cairo_t *cr;

static Atom XA_TARGETS;
static Atom clipatom;
static uint8_t *clip;
static size_t cliplen;

void
setclipboard(uint8_t *data, size_t len)
{
  if (clip != NULL) {
    free(clip);
  }

  clip = data;
  cliplen = len;
  XSetSelectionOwner(display, clipatom, win, CurrentTime);
}

uint8_t *
getclipboard(size_t *len)
{
  Atom actualtype;
  int actualformat;
  size_t bytesafter;
  uint8_t *ret = NULL;
  Window w;
  XEvent e;
  w = XGetSelectionOwner(display, clipatom);

  if (w == win) {
    *len = cliplen;
    return clip;
  } else if (w == BadWindow) {
    *len = 0;
    return NULL;
  }

  XConvertSelection(display, clipatom, XA_STRING, clipatom,
                    win, CurrentTime);

  while (true) {
    if (!XCheckTypedEvent(display, SelectionNotify, &e)) {
      continue;
    } else if (e.type != SelectionNotify) {
      continue;
    }

    if (e.xselection.property != None
        && e.xselection.target == XA_STRING) {
      XGetWindowProperty(display, win, e.xselection.property, 0,
                         16 * 1024, False, AnyPropertyType,
                         &actualtype, &actualformat, &cliplen,
                         &bytesafter, &ret);

      if (clip != NULL) {
        free(clip);
      }

      clip = malloc(cliplen);

      if (clip == NULL) {
        return NULL;
      }

      memmove(clip, ret, cliplen);
      XFree(ret);
      *len = cliplen;
      return clip;
    } else {
      return NULL;
    }
  }
}

static void
xresize(struct mace *m, int w, int h)
{
  width = w;
  height = h;
  cairo_xlib_surface_set_size(sfc, w, h);

  if (!maceresize(m, w, h)) {
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
    for (i = 0;
         i < sizeof(keymappings) / sizeof(keymappings[0]); i++) {
      if (keymappings[i].keysym == sym) {
        code = keymappings[i].unicode;
        break;
      }
    }
  }

  return code;
}

static size_t
encodekey(KeySym sym, uint8_t *s, size_t n, bool *special)
{
  int32_t code;
  
  *special = true;

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

  case XK_Left:
    strcpy((char *) s, "Left");
    break;

  case XK_Right:
    strcpy((char *) s, "Right");
    break;

  case XK_Up:
    strcpy((char *) s, "Up");
    break;

  case XK_Down:
    strcpy((char *) s, "Down");
    break;

  case XK_Page_Up:
    strcpy((char *) s, "PageUp");
    break;

  case XK_Page_Down:
    strcpy((char *) s, "PageDown");
    break;

  case XK_Home:
    strcpy((char *) s, "Home");
    break;

  case XK_End:
    strcpy((char *) s, "End");
    break;

  case XK_Return:
    strcpy((char *) s, "Return");
    break;

  case XK_Tab:
    strcpy((char *) s, "Tab");
    break;

  case XK_BackSpace:
    strcpy((char *) s, "BackSpace");
    break;

  case XK_Delete:
    strcpy((char *) s, "Delete");
    break;

  case XK_Escape:
    strcpy((char *) s, "Escape");
    break;

  default:
    *special = false;
    code = symtounicode(sym);

    if (code == 0) {
      return 0;
    }

    return utf8encode(s, n, code);
  }

  return strlen((char *) s);
}

static bool
xhandlekeypress(struct mace *m, XKeyEvent *e)
{
  size_t kn, nn = 0;
  uint8_t s[32], k[32];
  bool special;
  KeySym sym;
  sym = XkbKeycodeToKeysym(display, e->keycode, 0, 0);
  kn = encodekey(sym, k, sizeof(k), &special);

  if (!special) {
    sym = XkbKeycodeToKeysym(display, e->keycode, 0,
                             e->state & (ShiftMask | LockMask));
    kn = encodekey(sym, k, sizeof(k), &special);
  }

  if (special && (e->state & ShiftMask) != 0) {
    memmove(s + nn, "S-", 2);
    nn += 2;
    special = true;
  }

  if ((e->state & ControlMask) != 0) {
    memmove(s + nn, "C-", 2);
    nn += 2;
    special = true;
  }

  if ((e->state & Mod1Mask) != 0) {
    memmove(s + nn, "A-", 2);
    nn += 2;
    special = true;
  }

  if ((e->state & Mod4Mask) != 0) {
    memmove(s + nn, "M-", 2);
    nn += 2;
    special = true;
  }

  memmove(s + nn, k, kn);

  if (nn + kn > 0) {
    return handlekey(m, s, nn + kn, special);
  } else {
    return false;
  }
}

static bool
xhandlebuttonpress(struct mace *m, XButtonEvent *e)
{
  switch (e->button) {
  case 1:
  case 2:
  case 3:
    return handlebuttonpress(m, e->x, e->y, e->button);

  case 4:
    return handlescroll(m, e->x, e->y, 0, -m->font->lineheight);

  case 5:
    return handlescroll(m, e->x, e->y, 0, m->font->lineheight);

  default:
    return false;
  }
}

static bool
xhandlebuttonrelease(struct mace *m, XButtonEvent *e)
{
  switch (e->button) {
  case 1:
  case 2:
  case 3:
    return handlebuttonrelease(m, e->x, e->y, e->button);

  default:
    return false;
  }
}

static bool
xhandlemotion(struct mace *m, XMotionEvent *e)
{
  return handlemotion(m, e->x, e->y);
}

/* This is not a very good implimentation but it works. */

static void
xhandleselectionrequest(XSelectionRequestEvent *e)
{
  Atom targets[] = { XA_TARGETS, XA_STRING };
  XSelectionEvent *r;
  XEvent rr;
  r = &rr.xselection;
  r->type = SelectionNotify;
  r->requestor = e->requestor;
  r->selection = e->selection;
  r->target = e->target;
  r->time = CurrentTime;

  if (e->target == XA_TARGETS) {
    XChangeProperty(display, e->requestor, e->property,
                    XA_ATOM, 32, PropModeReplace,
                    (const uint8_t *) targets, sizeof(targets) / sizeof(Atom));
    r->property = e->property;
  } else if (e->target == XA_STRING) {
    r->property = e->property;
    XChangeProperty(display, e->requestor, e->property,
                    e->target, 8, PropModeReplace,
                    clip, cliplen);
  } else {
    r->property = None;
  }

  XSendEvent(display, e->requestor, True, 0, &rr);
}

static void
eventLoop(struct mace *m)
{
  bool redraw;
  XEvent e;

  while (m->running) {
    XNextEvent(display, &e);
    redraw = false;

    switch (e.type) {
    case ConfigureNotify:
      if (e.xconfigure.width != width
          || e.xconfigure.height != height) {
        xresize(m, e.xconfigure.width, e.xconfigure.height);
        redraw = true;
      }

      break;

    case SelectionRequest:
      xhandleselectionrequest(&e.xselectionrequest);
      break;

    case Expose:
      redraw = true;
      break;

    case KeyPress:
      redraw = xhandlekeypress(m, &e.xkey);
      break;

    case ButtonPress:
      redraw = xhandlebuttonpress(m, &e.xbutton);
      break;

    case ButtonRelease:
      redraw = xhandlebuttonrelease(m, &e.xbutton);
      break;

    case MotionNotify:
      redraw = xhandlemotion(m, &e.xmotion);
      break;

    /* Handle window close event? */
    case ClientMessage:
      return;
    }

    if (redraw) {
      cairo_push_group(cr);
      macedraw(m, cr);
      cairo_pop_group_to_source(cr);
      cairo_paint(cr);
      cairo_surface_flush(sfc);
    }
  }
}

int
displayinit(struct mace *m)
{
  int width, height;
  width = 800;
  height = 500;
  display = XOpenDisplay(NULL);

  if (display == NULL) {
    fprintf(stderr, "Failed to open X display!");
    return EXIT_FAILURE;
  }

  screen = DefaultScreen(display);
  XA_TARGETS = XInternAtom(display, "TARGETS", False);
  clipatom = XInternAtom(display, "CLIPBOARD", 0);
  win = XCreateSimpleWindow(display, RootWindow(display,
                            screen),
                            0, 0, width, height, 5, 0, 0);
  XSelectInput(display, win,
               ExposureMask | StructureNotifyMask
               | KeyPressMask | KeyReleaseMask | PointerMotionMask
               | ButtonPressMask | ButtonReleaseMask);
  XSetStandardProperties(display, win, "Mace", "Mace",
                         None, NULL, 0, NULL);
  XMapWindow(display, win);
  sfc = cairo_xlib_surface_create(display, win,
                                  DefaultVisual(display, screen),
                                  width, height);
  cr = cairo_create(sfc);
  xresize(m, width, height);
  cairo_push_group(cr);
  macedraw(m, cr);
  cairo_pop_group_to_source(cr);
  cairo_paint(cr);
  cairo_surface_flush(sfc);
	return EXIT_SUCCESS;
}

int
displayloop(struct mace *m)
{
  eventLoop(m);
  cairo_destroy(cr);
  cairo_surface_destroy(sfc);
  XDestroyWindow(display, win);
  XCloseDisplay(display);
  return EXIT_SUCCESS;
}
