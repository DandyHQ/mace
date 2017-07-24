#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>
#include <err.h>

#include "mace.h"

static int screen;

static int width, height;

static cairo_surface_t *sfc;
static cairo_t *cr;

static void
eventLoop(struct mace *m)
{

}

int
dodisplay(struct mace *m)
{
  int width, height;

  width = 800;
  height = 500;

  sfc = cairo_quartz_surface_create(CAIRO_FORMAT_A8, width, height);
  printf("surface create\n");

  cr = cairo_create(sfc);
  printf("cairo create\n");

  cairo_push_group(cr);
  printf("cairo push group\n");

  panedraw(m->pane, cr);
  printf("cairo pane draw\n");

  cairo_pop_group_to_source(cr);
  printf("pop group\n");
  cairo_paint(cr);
  printf("paint\n");
  cairo_surface_flush(sfc);
  printf("flush\n");

  eventLoop(m);
  printf("loop");

  cairo_destroy(cr);
  cairo_surface_destroy(sfc);

  return EXIT_SUCCESS;
}
