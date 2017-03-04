#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <X11/keysymdef.h>
#include <err.h>

#include "mace.h"

size_t width = 0;
size_t height = 0;
size_t bpp = 0;
char *buf = NULL;

struct pane *root = NULL;

bool
init(void)
{
  root = malloc(sizeof(struct pane));
  if (root == NULL) {
    fprintf(stderr, "Failed to allocate root pane!\n");
    return false;
  }

  root->ratio = 1.0f;
  root->split = SPLIT_none;
  root->a = NULL;
  root->b = NULL;

  root->cr = 255;
  root->cg = 0;
  root->cb = 0;

  return true;
}

void
resize(char *nbuf, size_t w, size_t h, size_t b)
{
  printf("resize to %ix%i\n", (int) w, (int) h);

  buf = nbuf;
  width = w;
  height = h;
  bpp = b;
}

void
drawpane(struct pane *p, size_t x, size_t y, size_t w, size_t h)
{
  size_t xx, yy, r;

  printf("draw pane at %i,%i %ix%i\n", x, y, w, h);

  switch (p->split) {
  case SPLIT_none:
    for (xx = 0; xx < w; xx++) {
      buf[(x + xx + (y + 0) * width) * bpp + 0] = 0;
      buf[(x + xx + (y + 0) * width) * bpp + 1] = 0;
      buf[(x + xx + (y + 0) * width) * bpp + 2] = 0;
      buf[(x + xx + (y + h-1) * width) * bpp + 0] = 0;
      buf[(x + xx + (y + h-1) * width) * bpp + 1] = 0;
      buf[(x + xx + (y + h-1) * width) * bpp + 2] = 0;

      for (yy = 1; yy < h-1; yy++) {
	buf[(x + xx + (y + yy) * width) * bpp + 0] = 255;
	buf[(x + xx + (y + yy) * width) * bpp + 1] = 255;
	buf[(x + xx + (y + yy) * width) * bpp + 2] = 255;
      }
    }
    
    for (yy = 0; yy < h; yy++) {
      buf[(x + 0 + (y + yy) * width) * bpp + 0] = 0;
      buf[(x + 0 + (y + yy) * width) * bpp + 1] = 0;
      buf[(x + 0 + (y + yy) * width) * bpp + 2] = 0;
      buf[(x + w-1 + (y + yy) * width) * bpp + 0] = 0;
      buf[(x + w-1 + (y + yy) * width) * bpp + 1] = 0;
      buf[(x + w-1 + (y + yy) * width) * bpp + 2] = 0;
    }
    break;

  case SPLIT_horz:
    r = (size_t) (p->ratio * (float) w);
    drawpane(p->a, x, y, r, h);
    drawpane(p->b, x + r, y, w - r, h);
    break;

  case SPLIT_vert:
    r = (size_t) (p->ratio * (float) h);
    drawpane(p->a, x, y, w, r);
    drawpane(p->b, x, y + r, w, h - r);
    break;
  }
}

void
redraw(void)
{
  printf("redraw\n");
  drawpane(root, 0, 0, width, height);
}

static struct pane *
findpanep(struct pane *p, size_t x, size_t y, size_t w, size_t h)
{
  size_t r;

  switch (p->split) {
  case SPLIT_none:
    printf("found\n");
    return p;

  case SPLIT_horz:
    r = (size_t) (p->ratio * (float) w);

    printf("horz\n");
    if (x < r) {
      printf("go a\n");
      return findpanep(p->a, x, y, r, h);
    } else {
      printf("go b\n");
      return findpanep(p->b, x - r, y, w - r, h);
    }
    
  case SPLIT_vert:
    r = (size_t) (p->ratio * (float) h);

    printf("vert\n");
    if (y < r) {
      printf("go a\n");
      return findpanep(p->a, x, y, w, r);
    } else {
      printf("go b\n");
      return findpanep(p->b, x, y - r, w, h - r);
    }

  default:
    return NULL;
  }
}

static struct pane *
findpane(size_t x, size_t y)
{
  printf("find pane\n");
  return findpanep(root, x, y, width, height);
}

bool
handlekeypress(unsigned int code)
{
  printf("keypress %i\n", code);
  return false;
}

bool
handlekeyrelease(unsigned int code)
{
  printf("keyrelease %i\n", code);
  return false;
}

bool
handlebuttonpress(int x, int y, unsigned int button)
{
  struct pane *h, *o, *n;
  split_t split;
  
  printf("press %i at %i,%i\n", button, x, y);

  h = findpane(x, y);
  if (h == NULL) {
    printf("failed to find pane!\n");
    return false;
  }

  if (button == 1) {
    printf("split horizontally\n");
    split = SPLIT_horz;
  } else if (button == 3) {
    printf("split vertically\n");
    split = SPLIT_vert;
  } else {
    return false;
  }

  printf("make new to store old\n");
  o = malloc(sizeof(struct pane));
  if (o == NULL) {
    fprintf(stderr, "Failed to allocate new pane!\n");
    return false;
  }

  o->split = SPLIT_none;
  o->cr = h->cr;
  o->cg = h->cg;
  o->cb = h->cb;
 
  printf("make new\n");
  n = malloc(sizeof(struct pane));
  if (n == NULL) {
    fprintf(stderr, "Failed to allocate new pane!\n");
    return false;
  }

  printf("old has color %i,%i,%i\n", o->cr, o->cg, o->cb);
  n->split = SPLIT_none;
  n->cr = o->cb;
  n->cg = o->cr;
  n->cb = o->cg;

  printf("new has color %i,%i,%i\n", n->cr, n->cg, n->cb);
  
  printf("put new old and new in old\n");
  h->a = o;
  h->b = n;
  h->ratio = 0.5f;
  h->split = split;

  printf("return true\n");

  return true;
}

bool
handlebuttonrelease(int x, int y, unsigned int button)
{
  printf("release %i at %i,%i\n", button, x, y);
  return false;
}

bool
handlemotion(int x, int y)
{
  return false;
  printf("motion %i,%i\n", x, y);
  return false;
}
