
.SUFFIXES: .c .h .o

CFLAGS := `pkg-config --cflags x11 freetype2` -I/usr/local/include \
        -fPIC -Wall -Wpointer-sign

LDFLAGS := `pkg-config --libs x11 freetype2` -L/usr/local/lib \
	-fPIC -lutf8proc

SRC := \
	main.c      \
	draw.c      \
	font.c      \
	mouse.c     \
	tab.c       \
	pane.c      \
	panedraw.c  \
	focus.c     \
	piece.c     \
	xmain.c

OBJECTS := $(SRC:%.c=%.o)

.c.o: mace.h
	$(CC) $(CFLAGS) -c $< -o $@

mace: $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS)

CLEAN := $(OBJECTS) mace mace.core
clean:
	rm -f $(CLEAN)
