/**
 * xclip.c
 * to compile and link: cc -o clip clip.c -lX11
 */
#include <stdio.h>
#include <stdlib.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>


//Convert an atom name in to a std::string
char *
GetAtomName(Display* disp, Atom a)
{
	if(a == None)
		return "None";
	else
		return XGetAtomName(disp, a);
}

struct Property
{
	unsigned char *data;
	int format, nitems;
	Atom type;
};


//This atom isn't provided by default
Atom XA_TARGETS;


//This fetches all the data from a property
struct Property read_property(Display* disp, Window w, Atom property)
{
	Atom actual_type;
	int actual_format;
	unsigned long nitems;
	unsigned long bytes_after;
	unsigned char *ret=0;

	int read_bytes = 1024;

	//Keep trying to read the property until there are no
	//bytes unread.
	do
	{
		if(ret != 0)
			XFree(ret);
		XGetWindowProperty(disp, w, property, 0, read_bytes, False, AnyPropertyType,
							&actual_type, &actual_format, &nitems, &bytes_after,
							&ret);

		read_bytes *= 2;
	}while(bytes_after != 0);

	fprintf(stderr, "Actual type: %s\n", GetAtomName(disp, actual_type));
	fprintf(stderr, "Actual format: %d\n", actual_format);
	fprintf(stderr, "Number of items: %ld\n", nitems);

	struct Property p = {ret, actual_format, nitems, actual_type};

	return p;
}



int main(int argc, char ** argv)
{

	Display* disp;
	Window root, w;
	int screen;
	XEvent e;

	//The usual Xinit stuff...
	disp = XOpenDisplay(NULL);
	screen = DefaultScreen(disp);
	root = RootWindow(disp, screen);


	//The first command line argument selects the buffer.
	//by default we use PRIMARY, the only other option
	//which is normally sensible is CLIPBOARD
	Atom sel = XInternAtom(disp, "CLIPBOARD", 0);


	//We need a target window for the pasted data to be sent to.
	//However, this does not need to be mapped.
	w = XCreateSimpleWindow(disp, root, 0, 0, 100, 100, 0, BlackPixel(disp, screen), BlackPixel(disp, screen));


	//This is a meta-format for data to be "pasted" in to.
	//Requesting this format acquires a list of possible
	//formats from the application which copied the data.
	XA_TARGETS = XInternAtom(disp, "TARGETS", False);

	//Request a list of possible conversions, if we're pasting.
	XConvertSelection(disp, sel, XA_TARGETS, sel, w, CurrentTime);

	XFlush(disp);


	Atom to_be_requested = None;
	int sent_request = 0;

	for(;;)
	{
		XNextEvent(disp, &e);

		if(e.type == SelectionNotify)
		{
			Atom target = e.xselection.target;

			if(e.xselection.property == None)
			{
				//If the selection can not be converted, quit with error 2.
				//If TARGETS can not be converted (nothing owns the selection)
				//then quit with code 3.
				return 2 + (target == XA_TARGETS);
			}
			else
			{
				struct Property prop = read_property(disp, w, sel);

				//If we're being given a list of targets (possible conversions)
				if(target == XA_TARGETS && !sent_request)
				{
					sent_request = 1;
					to_be_requested = XA_STRING;

					XConvertSelection(disp, sel, XA_STRING, sel, w, CurrentTime);
				}
				else if(target == to_be_requested)
				{
					//Dump the binary data
					fprintf(stderr, "Data begins:\n");
					fprintf(stderr, "--------\n");
					fwrite((char*)prop.data, prop.format/8, prop.nitems, stdout);
					fflush(stdout);
					fprintf(stderr, "\n--------\nData ends\n");

					return 0;
				}
				else return 0;

				XFree(prop.data);
			}
			fprintf(stderr, "\n");
		}
	}
}
