
.SUFFIXES: .c .h .o

CFLAGS := -I/usr/X11R6/include -I/usr/local/include \
	-I/usr/X11R6/include/freetype2 \
	-fPIC -Wall

LDFLAGS := -L/usr/X11R6/lib -L/usr/local/lib \
	-fPIC

LIBS := -lX11 -lXft -lfreetype -lz


SRC := \
	main.c      \
	draw.c      \
	mouse.c     \
	keys.c      \
	tab.c       \
	pane.c      \
	xmain.c

OBJECTS := $(SRC:%.c=%.o)

.c.o: mace.h
	$(CC) $(CFLAGS) -c $< -o $@

mace: $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS) \
		$(LIBS)

CLEAN := $(OBJECTS) mace
clean:
	rm -f $(CLEAN)
