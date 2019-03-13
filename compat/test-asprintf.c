#include <stdio.h>

int
test(char **ret) {
	return asprintf(ret, "test");
}
