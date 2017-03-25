
.SUFFIXES: .c .h .o

CFLAGS := -I/usr/X11R6/include -I/usr/local/include \
	-I/usr/X11R6/include/freetype2 \
        -fPIC -Wall -Wpointer-sign

LDFLAGS := -L/usr/X11R6/lib -L/usr/local/lib \
	-fPIC

LIBS := -lX11 -lXft -lfreetype -lz


SRC := \
	main.c      \
	draw.c      \
	font.c      \
	mouse.c     \
	tab.c       \
	pane.c      \
	focus.c     \
	line.c      \
	xmain.c

OBJECTS := $(SRC:%.c=%.o)

.c.o: mace.h
	$(CC) $(CFLAGS) -c $< -o $@

mace: $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS) \
		$(LIBS)

CLEAN := $(OBJECTS) mace mace.core
clean:
	rm -f $(CLEAN)
