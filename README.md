# ektest
> Simple unit testing and benchmarking utility for C

### Using eklipsed's test.h:

Try to include test.h last in your test file (but before your tests) so that
the keywords defined by test.h wont potentially mess up other headers.
I recommend turning on -O2 and maybe debugging symbols even if you are just
testing. Tests should have optimizations disabled by default leaving the
benchmark functions the only ones with optimizations.

### Keywords introduced by test.h:
- ```arrlen(a)``` get length of array
- ```test((optional) group_name, test_name)``` define test
- ```pass``` pass test
- ```fail``` fail test
- ```fail(fmt, ...)``` fail test with message
- ```test_log(fmt, ...)``` log message at line
- ```bench((optional) group_name, bench_name, num_iters)``` define test
- ```repeat(iters, n_times)``` run benchmark iters times and then that n_times
- ```onsetup(state_type) {}``` run code to setup benchmark with state
- ```oncleanup(state_type) {}``` run code to cleanup benchmark with state
- ```oniter(state_type) {}``` run code to run 1 iteration of benchmark with state

### Command line arguments:
The compiled binary has these options when running it
```
  -s,--silent       don't print test stdout
  -B,--no-bench     don't run the benchmarks
  -lfile            redirect stdout to file
```

### Example testing file:
```c
test(always_passes) {
	pass;
}
test(print_if_not_silent) {
	// Will print what line the log output was on
	test_log("I can make test output gross! But only if -s is NOT used!");
	pass;
}
test(always_fails) {
    // Will print what line the error occurred on
    fail;
}
test(always_fails2) {
    fail("failed in file: %s", "example.c");
}

test(math, 1) { // You can even name tests with just numbers!
    if (5 < 4) fail;
    pass;
}
test(math, 2) { // Tests get ran in the same order their defined.
    if (3 < 4) fail("happened after math_1");
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
```

