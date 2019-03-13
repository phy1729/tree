COMPAT = \
	asprintf.o \
	pledge.o \
	reallocarray.o \
	strtonum.o

all: libcompat.a
libcompat.a: ${COMPAT}
	${AR} ${ARFLAGS} $@ ${COMPAT}

.c.o:
	if ${CC} ${CLFAGS} -Werror -c test-$< >/dev/null 2>&1; then ${CC} -c -o $@ empty.c; else ${CC} ${CFLAGS} -c -o $@ $<; fi

clean:
	rm -f -- *.[ao]
