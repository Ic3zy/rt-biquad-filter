# rt_biquad header-only makefile

PREFIX ?= /usr/local
INCDIR = $(PREFIX)/include

.PHONY: all install uninstall test clean

all:
	@echo "rt_biquad is header-only, no compilation needed."
	@echo "just bring in include/rt_biquad.h to your project."
	@echo "run 'sudo make install' to drop it into your system headers."

install:
	@echo "installing header files to $(INCDIR)..."
	mkdir -p $(INCDIR)
	cp include/rt_biquad.h $(INCDIR)/

uninstall:
	@echo "removing headers..."
	rm -f $(INCDIR)/rt_biquad.h

test:
	@echo "running a quick compilation smoke test..."
	@echo '#include "rt_biquad.h"\nint main() { return 0; }' > .test.c
	$(CC) -O3 -msse4.2 -ffast-math -Wall -Wextra -Iinclude test/test_core.c -o .test_run -lm
	@./.test_run && echo "syntax looks good!"
	@rm -f .test.c .test_run

clean:
	@echo "cleaning up temp test artifacts..."
	rm -f .test.c .test_run
	rm -f *.o