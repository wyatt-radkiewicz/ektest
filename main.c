#include "test.h"

test(always_passes) {
	pass;
}
test(print_if_not_silent) {
	// Will print what line the log output was on
	log("I can make test output gross! But only if -s is NOT used!");
	pass;
}
test(always_fails) {
     // Will print what line the error occurred on
     //fail;
     pass;
}
test(always_fails2) {
     //fail("failed in file: %s", "example.c");
     pass;
}

test(math, 1) { // You can even name tests with just numbers!
     if (5 < 4) fail;
     pass;
}
test(math, 2) { // Tests get ran in the same order their defined.
     //if (3 < 4) fail("happened after math_1");
     pass;
}

bench(printf, 10000) oniter(void) {
	printf("iter: %llu\n", iter); // Can use iteration number in here!
}

const char *strtoull_strings[8] = {
	"0", "10", "-1234214", "5452345", "123412", "3124", "0543092", "-13",
};

// Here you can use repeat to make the iter variable only go to some number
// but still run the benchmark multiple times. Here iter will go from 0-7
bench(strtoull, repeat(8, 100000)) oniter(void) {
	strtoull(strtoull_strings[iter], NULL, 0);
}

// Here you can pass in a state type to the setup or cleanup functions to
// use state in the iterator if you need to
bench(state, 10000) onsetup(int) {
	*state = 13004;
} oniter(int) {
	printf("state: %d\n", *state);
}

