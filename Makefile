CFLAGS=-Wall -Wextra -ansi -pedantic -O3

all: befunge

clean:

distclean: clean
	rm -f befunge

test: tests/*

tests/*: befunge
	@./befunge "$@"/program <"$@"/input | diff -q - "$@"/output
	@echo "$@" passed

.PHONY: all clean distclean test tests/*
