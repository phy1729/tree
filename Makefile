CFLAGS ?= -O2
CFLAGS += -Wall -Wpedantic -Werror -Wformat=2 -Wshadow -Wpointer-arith \
	-Wcast-qual -Wstrict-aliasing=2 -Wwrite-strings -Wstack-protector

all: tree
tree: tree.o
	${CC} ${CFLAGS} -o $@ tree.o

clean:
	rm -f tree tree.o
