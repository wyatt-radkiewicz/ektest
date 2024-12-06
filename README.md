# ektest
> Simple unit testing and benchmarking utility for C

### Example testing file:
```c
#include "test.h"

test(always_passes) {
	pass;
}
test(always_fails) {
	fail;
}
test(asserts_always) {
	assert(5 == 4);
	pass;
}
test(asserts_message) {
	assert(5 == 4, "Wow this was dumb.");
	pass;
}

const volatile char *strings[] = {
	"0.5",
	"0.6",
	"0.7",
	"0.8",
};

bench_on(strtod1, strings, 10000) {
	// Use the built-in i parameter to get the current thing you are
	// looping over
	strtod((const char *)*i, NULL);
}
bench_for(strtod2, 10000) {
	strtod("0.4", NULL);
}
```

