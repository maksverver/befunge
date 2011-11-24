CFLAGS=-Wall -Wextra -ansi -pedantic -O3 -g

all: befunge

clean:

distclean: clean
	rm -f befunge

.PHONY: all clean distclean
