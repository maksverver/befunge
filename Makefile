CFLAGS=-Wall -Wextra -ansi -pedantic -O3 -g

all: befunge

clean:

distclean: clean
	rm -f befunge

test: tests/*

tests/*: befunge
	@./befunge "$@"/program <"$@"/input | diff - "$@"/output >/dev/null
	@echo "$@" passed

xtest: befunge
	@e=0; for t in tests/*; do \
		echo -n "$$(basename $$t)... "; \
		if ./befunge $$t/program <$$t/input | diff - $$t/output >/dev/null; \
			then echo passed; else echo failed; e=1; fi \
	done; exit $e
	exit 1

.PHONY: all clean distclean test tests/*
