#include <stdio.h>
#include <stdlib.h>

static inline void __test_assert(char *condition, char *message) {
	fprintf(stderr, "assert (%s) failed: %s\n", condition, message);
	exit(1);
}

#define test_assert(condition, message) (condition) ? (void) 0 : __test_assert(#condition, message);
