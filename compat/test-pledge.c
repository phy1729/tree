#include <unistd.h>

int
test(const char *promises, const char *execpromises) {
	return pledge(promises, execpromises);
}
