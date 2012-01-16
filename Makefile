CSRC := $(shell find src/ -name "*.c")
CHDR := $(shell find include/ -name "*.h")

COBJ := $(CSRC:.c=.o)

CC := clang

LIBS := $(shell sdl-config --libs)

CFLAGS  := -Wall -Wextra -std=c99 -pedantic $(shell sdl-config --cflags) -Iinclude/ -Wno-unused
LNFLAGS := $(LIBS)

EXE := nestorama

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
	@ack --type=cc 'XXX'
	@ack --type=cc 'TODO'
	@ack --type=cc 'FIXME'
loc:
	@ack --type=cc -f | xargs wc -l | sort -h

check-syntax:
	gcc -o nul -S $(CSRC) -I $(CHDR)

# requires sloccount
sloc:
	@sloccount . | grep '(SLOC)'

.PHONY= loc sloc todo all clean distclean debug
