CFLAGS ?= -O2
include ./Makefile.inc
CFLAGS += -Wall -Werror -Wformat=2 -Wshadow -Wpointer-arith \
	-Wcast-qual -Wstrict-aliasing=2 -Wwrite-strings -Wstack-protector \
	-Iinclude
LDFLAGS := -Lcompat ${LDFLAGS}
LIBCOMPAT = compat/libcompat.a

all: tree
tree: tree.o
	${CC} ${CFLAGS} ${LDFLAGS} -o $@ tree.o -lcompat

tree.o: ${LIBCOMPAT}

.PHONY: ${LIBCOMPAT}
${LIBCOMPAT}:
	( cd compat && ${MAKE} )

clean:
	rm -f tree tree.o
	( cd compat && ${MAKE} clean )
