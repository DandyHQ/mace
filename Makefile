

CFLAGS := -fPIC -std=c11 -Wall -Wpointer-sign \
	`pkg-config --cflags x11 freetype2` \
	-I/usr/local/include

LDFLAGS := -fPIC \
	`pkg-config --libs x11 freetype2` \
	-L/usr/local/lib -lutf8proc

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
