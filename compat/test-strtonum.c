#include <stdlib.h>

long long
test(const char *nptr, long long minval, long long maxval,
    const char **errstr) {
	return strtonum(nptr, minval, maxval, errstr);
}
