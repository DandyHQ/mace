#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>
#include <err.h>
#include <unistd.h> // remove

#import <AppKit/AppKit.h>

#include <cairo-quartz.h>

#include "mace.h"

static int screen;

static int width, height;

static cairo_surface_t *sfc;
static cairo_t *cr;

static void
eventLoop(struct mace *m)
{
  sleep(3);
}

int
dodisplay(struct mace *m)
{
  int width, height;
  CGContextRef cg_context;

  width = 800;
  height = 500;

  cg_context = [[NSGraphicsContext currentContext] graphicsPort];

  sfc = cairo_quartz_surface_create_for_cg_context(
          cg_context, width, height);

  cr = cairo_create(sfc);

  cairo_push_group(cr);

  panedraw(m->pane, cr);

  cairo_pop_group_to_source(cr);
  cairo_paint(cr);
  cairo_surface_flush(sfc);

  eventLoop(m);

  cairo_destroy(cr);
  cairo_surface_destroy(sfc);

  return EXIT_SUCCESS;
}
