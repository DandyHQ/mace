#include <string.h>
#include "mace.h"

static uint8_t message[] =
  "Welcome to the mace text editor."
  "\n\n" 
  "I am afraid that mace is not the most intuitive so I will attempt "
  "to guide you through its usage."
  "\n\n"
  "To start, if you type you should end up adding text to the end of "
  "this message. You can click somewhere to move the cursor to that "
  "point and any more typing should go there. This should be fairly "
  "normal."
  "\n\n"
  "This alone cause mace to count as a functioning text editor, abit "
  "not a very useful or interesting one. Next I will introduce you "
  "to commands."
  "\n\n"
  "I'm going to assume you are in the root directory of the mace "
  "repository. Now select the text on the next line, be careful not "
  "to select any white space on either side of it:"
  "\n\n"
  "init.lua"
  "\n\n"
  "Now right click open: open"
  "\n\n"
  "What should have happened is that you now have a new tab, fulled "
  "with the contents of mace's init file. What happned under the "
  "hood is that mace found the word that you right clicked in (open) "
  "And ran the function 'open' in mace's lua runtime. The function "
  "looks at the selections you have made in the current text box and "
  "opens new tabs for each of these files."
  "\n\n"
  "There are other commands in mace, though currently not many. The "
  "only ones so far are open, save, eval and test. test is not very "
  "interesting, it is only there to check that the lua "
  "implimentation is doing what it should be. eval however is "
  "interesting. With the eval command you can add new functions or "
  "evaluate lua code in runtime without editing a file."
  "\n\n"
  "I'll give you an example. Currently the only way to scroll in "
  "mace is by using a scroll wheel, which is pretty slow and "
  "frustraiting. Say you decide you want to be able to scroll by "
  "50px a click. So lets add a function to do just that. Select the "
  "text below and click eval :"
  "\n\n"
  "function scroll()\n"
  "    tb = mace.focus.tab.main\n"
  "    tb.yoff = tb.yoff + 50\n"
  "end\n"
  "\n\n"
  "Now type scroll in the action bar of this tab, or another tab. "
  "Now right click on it. What should happen is you tabs main text "
  "box should have scrolled down 50 pixels. Hopefully you can see "
  "the use of this. Have a look in init.lua for some examples of "
  "scripting mace."
  "\n\n"
  "If you edit this file and want to save it you can right click on "
  "save, which you could type yourself anywhere in the tab, or it "
  "could be the save word that is by default in the action bar of "
  "every newly created tab. The file will be saved as whatever text "
  "is in the tabs action bar before the colon."
  "\n\n"
  "I am afraid this tutorial was probably not very helpfull. I wish "
  "you good luck, hopefully you will be able to figure it out."
  "\n\n"
  ;


struct mace *
macenew(void)
{
  const uint8_t name[] = "Mace";
  struct sequence *s;
  struct mace *m;
  struct tab *t;
  uint8_t *buf;

  m = calloc(1, sizeof(struct mace));
  if (m == NULL) {
    return NULL;
  }

  m->font = fontnew();
  if (m->font == NULL) {
    macefree(m);
    return NULL;
  }

  m->lua = luanew(m);
  if (m->lua == NULL) {
    macefree(m);
    return NULL;
  }

  buf = malloc(sizeof(message));
  if (buf == NULL) {
    macefree(m);
    return NULL;
  }

  memmove(buf, message, sizeof(message));
  
  s = sequencenew(buf, sizeof(message));
  if (s == NULL) {
    free(buf);
    macefree(m);
    return NULL;
  }

  m->pane = panenew(m);
  if (m->pane == NULL) {
    macefree(m);
    return NULL;
  }

  t = tabnew(m, name, strlen((const char *) name), s);
  if (t == NULL) {
    macefree(m);
    return NULL;
  }

  m->focus = t->main;
  paneaddtab(m->pane, t, 0);
  m->pane->focus = t;

  m->running = true;

  luaruninit(m->lua);
  
  return m;
}

void
macequit(struct mace *m)
{
  m->running = false;
}

void
macefree(struct mace *m)
{
  if (m->pane != NULL) {
    panefree(m->pane);
  }

  if (m->lua != NULL) {
    luafree(m->lua);
  }

  if (m->font != NULL) {
    fontfree(m->font);
  }

  free(m);
}

static struct pane *
findpane(struct mace *mace, int x, int y)
{
  struct pane *p;

  p = mace->pane;

  /* Do search if there were more panes */
  
  return p;
}

bool
handlebuttonpress(struct mace *mace, int x, int y, int button)
{
  struct pane *p = findpane(mace, x, y);

  x -= p->x;
  y -= p->y;
  
  if (y < mace->font->lineheight) {
    return tablistbuttonpress(p, x, y, button);
  } else {
    return tabbuttonpress(p->focus, x, y - mace->font->lineheight,
			  button);
  }
}

bool
handlebuttonrelease(struct mace *mace, int x, int y, int button)
{
  struct tab *f;

  if (mace->focus == NULL) {
    return false;
  } else {
    f = mace->focus->tab;
  }

  return tabbuttonrelease(f, x - f->x, y - f->y, button);
}

bool
handlemotion(struct mace *mace, int x, int y)
{
  struct tab *f;

  if (mace->focus == NULL) {
    return false;
  } else {
    f = mace->focus->tab;
  }

  return tabmotion(f, x - f->x, y - f->y);
}

bool
handlescroll(struct mace *mace, int x, int y, int dx, int dy)
{
  struct pane *p = findpane(mace, x, y);

  x -= p->x;
  y -= p->y;
  
  if (y < mace->font->lineheight) {
    return tablistscroll(p, x, y, dx, dy);
  } else {
    return tabscroll(p->focus, x, y - mace->font->lineheight, dx, dy);
  }
}

bool
handletyping(struct mace *mace, uint8_t *s, size_t n)
{
  if (mace->focus == NULL) {
    return false;
  } else {
    return textboxtyping(mace->focus, s, n);
  }
}

bool
handlekeypress(struct mace *mace, keycode_t k)
{
  if (mace->focus == NULL) {
    return false;
  } else {
    return textboxkeypress(mace->focus, k);
  }
}

bool
handlekeyrelease(struct mace *mace, keycode_t k)
{
  if (mace->focus == NULL) {
    return false;
  } else {
    return textboxkeyrelease(mace->focus, k);
  }
}
