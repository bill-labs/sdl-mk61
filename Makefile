PREFIX ?= /opt/gnux
DESTDIR ?=
BINDIR ?= $(PREFIX)/bin
DATADIR ?= $(PREFIX)/share/sdl-mk61/data

SDL_CFLAGS := $(shell sdl-config --cflags)
SDL_LIBS := $(shell sdl-config --libs)
SDL_IMAGE_LIBS := -lSDL_image

CC = gcc
CFLAGS = -O3 -fomit-frame-pointer -Wall -Wextra -std=gnu99 -D_UNIXWARE -DDATADIR=\"$(DATADIR)\" -I. -Isrc -Imicrolib $(SDL_CFLAGS)
LDFLAGS = -L$(PREFIX)/lib -L$(PREFIX)/X11R6/lib -L/usr/X11R6/lib \
		-Wl,-rpath,$(PREFIX)/lib -Wl,-rpath,$(PREFIX)/X11R6/lib -Wl,-rpath,/usr/X11R6/lib
LDLIBS = $(SDL_LIBS) $(SDL_IMAGE_LIBS) -lgnux

SRCS = \
	src/cmcu13.c \
	src/cmem.c \
	src/cgraphics.c \
	src/display.c \
	src/emu145.c \
	src/mkenv.c \
	sdl_microlib.c \
	sdl_main.c

OBJS = $(SRCS:.c=.o)
TARGET = sdl-mk61

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS) $(LDLIBS)

clean:
	rm -f $(OBJS) $(TARGET) mk61-sdl

install: $(TARGET)
	mkdir -p $(DESTDIR)$(BINDIR)
	mkdir -p $(DESTDIR)$(DATADIR)
	cp $(TARGET) $(DESTDIR)$(BINDIR)/
	cp data/*.png $(DESTDIR)$(DATADIR)/

.PHONY: all clean install
