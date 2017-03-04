
.SUFFIXES: .c .h .o

CFLAGS := -I/usr/X11R6/include -I/usr/local/include \
	-fPIC -Wall

LDFLAGS := -L/usr/X11R6/lib -L/usr/local/lib \
	-fPIC

LIBS := -lX11 -lXft


SRC := \
	xmain.c     \
	main.c

OBJECTS := $(SRC:%.c=%.o)

.c.o: mace.h
	$(CC) $(CFLAGS) -c $< -o $@

mace: $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS) \
		$(LIBS)

CLEAN := $(OBJECTS) mace
clean:
	rm -f $(CLEAN)
