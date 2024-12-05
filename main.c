#include "test.h"

test(hello_world) {
	pass;
}
test(asdf, hello_world) {
	fail;
}
test(asdf, asdfasdf) {
	fail("ligma balls");
}
bench(strings, ejcmp, 25) oniter { }
bench(strings, ejcmp2, 25000) oniter { }
bench(cmp, ejcmp2, 25000) oniter { }

