CSRC := $(shell find src/ -name "*.c")
CHDR := $(shell find include/ -name "*.h")

COBJ := $(CSRC:.c=.o)

CC := clang

LIBS := $(shell sdl-config --libs)

CFLAGS  := -Wall -Wextra -std=c99 -pedantic $(shell sdl-config --cflags) -Iinclude/ -Wno-unused
LNFLAGS := $(LIBS)

EXE := nes

all: $(COBJ) $(CHDR) $(EXE)

$(EXE): $(COBJ)
	$(CC) $(COBJ) $(LNFLAGS) -o$(EXE)

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

debug:
	$(MAKE) all "CFLAGS=$(CFLAGS) -g -O0"

clean:
	rm -f $(COBJ)

todo:
	@find . -type f | xargs grep -n -i "TODO"
	@find . -type f | xargs grep -n -i "FIXME"

loc:
	@wc -l ./*.[ch]

check-syntax:
	gcc -o nul -S $(CSRC) -I $(CHDR)

# requires sloccount
sloc:
	@sloccount . | grep '(SLOC)'

.PHONY= loc sloc todo all clean distclean debug
