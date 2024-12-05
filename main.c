#include "test.h"

test(hello_world) { }
test(asdf, hello_world) { }
test(asdf, asdfasdf) { }
bench(strings, ejcmp, 25) oniter { }
bench(strings, ejcmp2, 25000) oniter { }
bench(cmp, ejcmp2, 25000) oniter { }


