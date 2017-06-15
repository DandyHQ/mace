#include <string.h>
#include "mace.h"
#include "config.h"

static uint8_t message[] =
  "Welcome to the mace text editor."
  "\n\n" 
  "Below is a basic guide on how to use Mace."
  "\n\n"
  "To start, if you type you should end up adding text to the end of "
  "this message. You can click somewhere to move the cursor to that "
  "point and any more typing should go there. This should be fairly "
  "normal."
  "\n\n"
  "This alone is enough for Mace to count as a functioning text editor, albeit "
  "not a very useful or interesting one. Next I will introduce you "
  "to commands."
  "\n\n"
  "Now select the text on the next line, be careful not "
  "to select any white space on either side of it:"
  "\n\n"
  MACE_PATH
  "/macerc.lua"
  "\n\n"
  "Now right click open: open"
  "\n\n"
  "What should have happened is that you now have a new tab, filled "
  "with the contents of your default init file. What happened under the "
  "hood is that Mace found the word that you right clicked on (open) "
  "And ran the function 'open' in Mace's lua runtime. The function "
  "looks at the selections you have made in the current text box and "
  "opens new tabs for each of these files."
  "\n\n"
  "In the action bar above, you can see a variety of other commands. "
  "The save, open, close, cut, copy, paste and quit all do what you "
  "would expect. With the eval command you can add new functions or evaluate "
  "lua code in runtime without editing a file."
  "\n\n"
  "I'll give you an example. Currently the only way to scroll in "
  "Mace is by using a scroll wheel, which is pretty slow and "
  "frustraiting. Say you decide you want to be able to scroll by "
  "50px a click. So let's add a function to do just that. Select the "
  "text below and click eval in your action bar:"
  "\n\n"
  "function scroll()\n"
  "    tb = mace.mousefocus.tab.main\n"
  "    tb.yoff = tb.yoff + 50\n"
  "end"
  "\n\n"
  "Now type scroll in the action bar of this tab, or another tab. "
  "Now right click on it. What should happen is your tab's main text "
  "box should have scrolled down 50 pixels. Hopefully you can see "
  "the use of this. Have a look in " MACE_PATH "/mace.lua for some "
  "examples of scripting Mace."
  "\n\n"
  "If you edit a file and want to save it you can right click on "
  "save, which you could type yourself anywhere in the tab, or it "
  "could be the 'save' that is by default in the action bar of "
  "every newly created tab. The file will be saved as whatever text "
  "is in the tabs action bar before the colon."
  "\n\n"
  "I hope this guide was helpful. Have fun using Mace!"
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
  size_t l;

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

  l = snprintf((char *) buf, sizeof(message), "%s", message);
  
  s = sequencenew(buf, l);
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

  m->keyfocus = m->mousefocus= t->main;
  paneaddtab(m->pane, t, 0);
  m->pane->focus = t;

  m->running = true;

  lualoadrc(m->lua);
  
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

  /* Once we had multiple panes in find the one that bounds x,y. */
  
  return p;
}

bool
handlebuttonpress(struct mace *mace, int x, int y, int button)
{
  struct pane *p = findpane(mace, x, y);

  x -= p->x;
  y -= p->y;
  
  if (y < mace->font->lineheight) {
		mace->mousefocus = NULL;
    return tablistbuttonpress(p, x, y, button);
  } else {
		return tabbuttonpress(p->focus, x, y - mace->font->lineheight, button);
	}
}

bool
handlebuttonrelease(struct mace *mace, int x, int y, int button)
{
	if (mace->mousefocus != NULL) {
		x -= mace->mousefocus->tab->x;
		y -= mace->mousefocus->tab->y;

		if (mace->mousefocus->tab->main == mace->mousefocus) {
			y -= mace->mousefocus->tab->action->height;
		}
		
  	return textboxbuttonrelease(mace->mousefocus, x, y, button);
	} else {
		return false;
	}
}

bool
handlemotion(struct mace *mace, int x, int y)
{
	if (mace->mousefocus != NULL) {
  	x -= mace->mousefocus->tab->x;
		y -= mace->mousefocus->tab->y;

		if (mace->mousefocus->tab->main == mace->mousefocus) {
			y -= mace->mousefocus->tab->action->height;
		}

		return textboxmotion(mace->mousefocus, x, y);
	} else {
		return false;
	}

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
  if (mace->keyfocus != NULL) {
    return textboxtyping(mace->keyfocus, s, n);
  } else {
		return false;
	}
}

bool
handlekeypress(struct mace *mace, keycode_t k)
{
  if (mace->keyfocus != NULL) {
    return textboxkeypress(mace->keyfocus, k);
  } else {
		return false;
	}
}

bool
handlekeyrelease(struct mace *mace, keycode_t k)
{
  if (mace->keyfocus != NULL) {
    return textboxkeyrelease(mace->keyfocus, k);
  } else {
		return false;
	}
}
