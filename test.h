#pragma once
#pragma GCC optimize(2)
// ektest (version 1.1.0)
// Simple unit testing and benchmarking utility for C
//
// Using eklipsed's test.h:
//
// Try to include test.h last in your test file (but before your tests) so that
// the keywords defined by test.h wont potentially mess up other headers.
// I recommend turning on -O2 and maybe debugging symbols even if you are just
// testing. Tests should have optimizations disabled by default leaving the
// benchmark functions the only ones with optimizations.
//
// Keywords introduced by test.h:
// - arrlen(a): get length of array
// - test((optional) group_name, test_name): define test
// - pass: pass test
// - fail: fail test
// - fail(fmt, ...): fail test with message
// - test_log(fmt, ...): log message at line
// - bench((optional) group_name, bench_name, num_iters): define test
// - repeat(iters, n_times): run benchmark iters times and then that n_times
// - onsetup(state_type) {}: run code to setup benchmark with state
// - oncleanup(state_type) {}: run code to cleanup benchmark with state
// - oniter(state_type) {}: run code to run 1 iteration of benchmark with state
//
// Command line arguments:
// The compiled binary has these options when running it
//   -s,--silent	don't print test stdout
//   -B,--no-bench	don't run the benchmarks
//   -lfile		redirect stdout to file
//
// Example testing file:
// test(always_passes) {
// 	pass;
// }
// test(print_if_not_silent) {
// 	// Will print what line the log output was on
// 	log("I can make test output gross! But only if -s is NOT used!");
// 	pass;
// }
// test(always_fails) {
//	// Will print what line the error occurred on
//	fail;
// }
// test(always_fails2) {
//	fail("failed in file: %s", "example.c");
// }
//
// test(math, 1) { // You can even name tests with just numbers!
//	if (5 < 4) fail;
//	pass;
// }
// test(math, 2) { // Tests get ran in the same order their defined.
//	if (3 < 4) fail("happened after math_1");
//	pass;
// }
//
// bench(printf, 10000) oniter(void) {
// 	printf("iter: %llu\n", iter); // Can use iteration number in here!
// }
//
// const char *strtoull_strings[8] = {
// 	"0", "10", "-1234214", "5452345", "123412", "3124", "0543092", "-13",
// };
//
// // Here you can use repeat to make the iter variable only go to some number
// // but still run the benchmark multiple times. Here iter will go from 0-7
// bench(strtoull, repeat(8, 100000)) oniter(void) {
// 	strtoull(strtoull_strings[iter], NULL, 0);
// }
//
// // Here you can pass in a state type to the setup or cleanup functions to
// // use state in the iterator if you need to
// bench(state, 10000) onsetup(int) {
// 	*state = 13004;
// } oniter(int) {
// 	printf("state: %d\n", *state);
// }
//

// Include everything the user might need (and we need)
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Used to redirect stdout
#include <unistd.h>

// Used to get user and system time from benchmarks
#include <sys/resource.h>
#include <sys/time.h>
#include <time.h>

// Nice macro to have in tests sometimes (not used in this file though)
#ifndef arrlen
# define arrlen(a) (sizeof(a) / sizeof((a)[0]))
#endif

// Default column of test results
#ifndef test_padding
# define test_padding 32
#endif

// Macro to make a function run automatically
// Use the ctor function as a stub to just add the real function.
// Use the ctor priorty to enforce run order (0-100 reserved)
#define _test_autorun __attribute__((constructor(__COUNTER__ + 101)))

// Macro to ensure a function is built at optlevel 0 so that benchmarks can be
// at optlevel2 or above and tests can be at debug optlevel 0
#ifdef __clang__
# define _test_optnone __attribute__((optnone))
#else
# define _test_optnone __attribute__((optimize (0))
#endif

// Creates a test in _group with _name
#define _test_group(_group, _name)                                             \
	/* Forward declare the test so we can add its function pointer */      \
	static _test_fn		  _test_##_group##_##_name;                    \
	static _test_autorun void _test_add_##_group##_##_name(void) {         \
		_test_nodes_add(                                               \
			&_test_tests, #_group,                                 \
			&(_test_t){                                            \
				.fn = _test_##_group##_##_name,                \
				.name = #_name,                                \
			},                                                     \
			sizeof(_test_t));                                      \
	}                                                                      \
	/* The actual code of the test.                                        \
	 * _out already is setup for a passing test. */                        \
	static _test_optnone uint8_t *_test_##_group##_##_name(void)

// Macros to allow the user to just pass in test name or also include group
#define _test(_name) _test_group(_, _name)
#define _test_pick(_1, _0, _name, ...) _name
#define test(...) _test_pick(__VA_ARGS__, _test_group, _test)(__VA_ARGS__)

// Can be used in test or benchmark functions
#define test_log(msg, ...)                                                     \
	(printf("\n[line %d]: " msg "\n", __LINE__, ##__VA_ARGS__))

// Pass and fail macros
#define pass return NULL
#define fail return __LINE__ + (uint8_t *)_test_fail

// Used in this library inplace of normal printf and printing to stderr
#define _testp(...) fprintf(_test_realout, __VA_ARGS__)
#define _testep(...) fprintf(stderr, __VA_ARGS__)

// Internal macro to create a benchmark in _group with _name and n _iters
#define _test_bench_group(_group, _name, _iters)                               \
	static _test_autorun void _test_bench_add_##_group##_##_name(void) {   \
		_test_bench_t *bench = _test_nodes_add(                        \
			&_test_benches, #_group,                               \
			&(_test_bench_t){                                      \
				.setup = NULL,                                 \
				.cleanup = NULL,                               \
				.iter = NULL,                                  \
				.name = #_name,                                \
				.iters = 0,                                    \
				.mult = 0,                                     \
				.statesz = 0,                                  \
			},                                                     \
			sizeof(_test_bench_t));                                \
		/* Set the iters to either a single int or a mult range */     \
		_Generic(                                                      \
			_iters,                                                \
			_test_bench_range_t: _test_bench_mult2,                \
			default: _test_bench_mult1)(bench, _iters);            \
		/* To my knowledge you can only use unique names in macros     \
		 * in the macro itself but not in other macros, so here we     \
		 * programmatically save the function pointers for when the    \
		 * user calls the other macros to set the func. pointers */    \
		_test_bench_setup = &bench->setup;                             \
		_test_bench_cleanup = &bench->cleanup;                         \
		_test_bench_iter = &bench->iter;                               \
		_test_bench_statesz = &bench->statesz;                         \
	}

// Use the bench macro and pick default group argument if its left out
#define _test_bench(_name, _iters) _test_bench_group(_, _name, _iters)
#define _test_bench_pick(_2, _1, _0, _name, ...) _name
#define bench(...)                                                             \
	_test_bench_pick(__VA_ARGS__, _test_bench_group, _test_bench)(         \
		__VA_ARGS__)

// Used to repeat a benchmark NxMULT times with iter only going through N range
#define repeat(_n, _mult) ((_test_bench_range_t){.iters = _n, .mult = _mult})

// Normal concatination macros to make sure arguments are expanded
#define _test_concat2(_1, _2) _1##_2
#define _test_concat(_1, _2) _test_concat2(_1, _2)
#define _test_appendline(_name) _test_concat(_name, __LINE__)

// Functions used to set benchmark functions (setup, cleanup, iter)
#define onsetup(_state_ty)                                                     \
	;                                                                      \
	static void _test_appendline(_test_bench_setup)(_state_ty *);          \
	static _test_autorun void _test_appendline(_test_bench_sets)(void) {   \
		*_test_bench_setup = (_test_bench_setup_fn *)_test_appendline( \
			_test_bench_setup);                                    \
		*_test_bench_statesz = _Generic(                               \
			(typeof(_state_ty) *){0},                              \
			void *: 0,                                             \
			default: sizeof(_state_ty));                           \
	}                                                                      \
	static void _test_appendline(_test_bench_setup)(_state_ty *const state)
#define oncleanup(_state_ty)                                                   \
	;                                                                      \
	static void _test_appendline(_test_bench_cleanup)(_state_ty *);        \
	static _test_autorun void _test_appendline(_test_bench_sets)(void) {   \
		*_test_bench_cleanup =                                         \
			(_test_bench_cleanup_fn *)_test_appendline(            \
				_test_bench_cleanup);                          \
	}                                                                      \
	static void _test_appendline(_test_bench_cleanup)(                     \
		_state_ty *const state)
#define oniter(_state_ty)                                                      \
	;                                                                      \
	static void _test_appendline(_test_bench_iter)(_state_ty *, uint64_t); \
	static _test_autorun void _test_appendline(_test_bench_sets)(void) {   \
		*_test_bench_iter = (_test_bench_iter_fn *)_test_appendline(   \
			_test_bench_iter);                                     \
	}                                                                      \
	static void _test_appendline(                                          \
		_test_bench_iter)(_state_ty *const state, const uint64_t iter)

// Test function prototype.
// Returns NULL if test succeeded.
typedef uint8_t *(_test_fn)(void);

// This function gets run (or address returned) if the test failed
static void *_test_fail(const char *fmt, ...);

// Structure used to defined tests
typedef struct _test {
	_test_fn   *fn;
	const char *name; // Name of the test without "test" prefix
} _test_t;

// Benchmarks and tests are grouped so this structure is just here to store
// either of them
typedef struct _test_group {
	const char *name;
	void	   *nodes;
	size_t	    len, capacity;
} _test_group_t;

// Loops through each node in _group with _name (pointer to _type)
#define _test_group_foreach(_group, _type, _name)                              \
	for (_type *_name = (_group)->nodes;                                   \
	     _name != ((_type *)(_group)->nodes) + (_group)->len; _name++)

// A database of groups of something to run (tests, benchmarks)
typedef struct _test_nodes {
	_test_group_t *groups;
	size_t	       len;
	size_t	       nodesz;
	size_t	       capacity;
} _test_nodes_t;

// Benchmark function prototypes
typedef void(_test_bench_setup_fn)(void *test_state);
typedef void(_test_bench_cleanup_fn)(void *test_state);
typedef void(_test_bench_iter_fn)(void *test_state, size_t iter);

// Structure used to defined benchmarks
typedef struct _test_bench {
	const char	       *name;
	_test_bench_setup_fn   *setup;
	_test_bench_cleanup_fn *cleanup;
	_test_bench_iter_fn    *iter;
	size_t			statesz;
	uint64_t		mult;  // How many times we bench on iter
	uint64_t		iters; // How many times iter should be called
} _test_bench_t;

// Used to set mult and iters in test_bench
typedef struct _test_bench_range {
	uint64_t iters;
	uint64_t mult;
} _test_bench_range_t;

// Database of tests and benchmarks that is built up with the ctor functions
static _test_nodes_t _test_tests, _test_benches;

// Used in benchmark macros to set the current benchmark's callbacks
static _test_bench_setup_fn   **_test_bench_setup;
static _test_bench_cleanup_fn **_test_bench_cleanup;
static _test_bench_iter_fn    **_test_bench_iter;
static size_t		       *_test_bench_statesz;

// Used to globally redirect stdout and stderr
static FILE *_test_realout, *_test_stdout;

// Command line flags
static bool	   _test_silent;	// Don't print test's stdout
static bool	   _test_bench_disable; // Don't run benchmarks
static const char *_test_logfile;	// Where to put test stdout to

// Add a node (test, benchmark) to a nodes database
static void *_test_nodes_add(
	_test_nodes_t *nodes, const char *name, void *node, size_t nodesz) {
	_test_group_t *group;

	// Try to find existing group
	for (size_t i = 0; i < nodes->len; i++) {
		if (strcmp(name, nodes->groups[i].name) == 0) {
			group = nodes->groups + i;
			goto add;
		}
	}

	// First run though, initialize the database
	if (!nodes->capacity) {
		*nodes = (_test_nodes_t){
			.capacity = 8,
			.groups = malloc(sizeof(*nodes->groups) * 8),
			.nodesz = nodesz,
			.len = 0,
		};
	}

	// Increase groups array if needed
	if (nodes->len >= nodes->capacity) {
		nodes->capacity *= 2;
		nodes->groups =
			realloc(nodes->groups,
				sizeof(*nodes->groups) * nodes->capacity);
	}

	// Add the group
	nodes->groups[nodes->len] = (_test_group_t){
		.name = name,
		.len = 0,
		.capacity = 8,
		.nodes = malloc(nodes->nodesz * 8),
	};
	group = nodes->groups + nodes->len++;

add:
	// Increase nodes length if needed
	if (group->len >= group->capacity) {
		group->capacity *= 2;
		group->nodes =
			realloc(group->nodes, nodes->nodesz * group->capacity);
	}

	// Set the node and return its address
	memcpy((void *)((uintptr_t)group->nodes + group->len * nodes->nodesz),
	       node, nodesz);
	return (void *)((uintptr_t)group->nodes + group->len++ * nodes->nodesz);
}

// Free resources used by node database
static void _test_nodes_free(_test_nodes_t *nodes) {
	if (!nodes->capacity) return; // Exit early if it was never initialized
	for (size_t i = 0; i < nodes->len; i++) free(nodes->groups[i].nodes);
	free(nodes->groups);
}

// Print (left aligned) with atleast to characters (padded with spaces)
static void _testppadded(int to, const char *str, ...) {
	va_list args;
	va_start(args, str);
	to -= vfprintf(_test_realout, str, args);
	for (; to > 0; to--) putc(' ', _test_realout);
	va_end(args);
}

// Run a test and return if it succeeded.
static bool _test_run_test(const char *group, const _test_t *test) {
	_testppadded(test_padding, "%s %s:", group, test->name);
	_testp(" ...");

	// Run the test
	uint8_t *res = test->fn();
	size_t	 failline = (uintptr_t)res - (uintptr_t)_test_fail;
	bool	 passed = res == NULL;

	// Print new status
	_testp("\r");
	_testppadded(test_padding, "%s %s:", group, test->name);
	if (passed) _testp(" \x1B[37;42m[PASS]\x1B[0m\n");
	else _testp(" \x1B[30;41m[FAIL (line %zu)]\x1B[0m\n", failline);

	fflush(stdout); // Make sure bench stdout is flushed
	fflush(stderr); // Make sure bench stderr is flushed
	return passed;
}

// Fails a currently running test
static void *_test_fail(const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	fprintf(stderr, "[FAIL MSG]: ");
	vfprintf(stderr, fmt, args);
	fprintf(stderr, "\n");
	va_end(args);
	return _test_fail;
}

// Print pretty results of test suite
static void _testpres(const char *name, size_t passed, size_t ran) {
	_testp("%s %zu/%zu test%spassed (%zu%%)\n", name, passed, ran,
	       passed == 1 ? " " : "s ", passed * 100 / ran);
}

// Run a group of tests.
static void _test_run_test_group(
	_test_group_t *group, bool run_unnamed, size_t *total_passed,
	size_t *total_ran) {
	const char *name = strcmp(group->name, "_") == 0 ? "" : group->name;
	size_t	    npassed = 0;

	if (run_unnamed && name[0] != '\0') return;
	if (!run_unnamed && name[0] == '\0') return;

	_test_group_foreach(group, _test_t, j) {
		npassed += _test_run_test(name, j);
	}

	*total_passed += npassed, *total_ran += group->len;
	_testpres(name, npassed, group->len);
}

// Run all tests
static bool _test_run_tests(void) {
	struct {
		size_t npassed, nran;
	} total = {0};

	// Try and run unnamed group first and if not, then run other groups
	_testp("tests:");
	for (size_t i = 1; i < 2; i--) {
		_testp("\n");
		for (size_t j = 0; j < _test_tests.len; j++) {
			_test_run_test_group(
				_test_tests.groups + j, i, &total.npassed,
				&total.nran);
		}
	}

	// Overall results
	if (_test_tests.len > 1) _testpres("", total.npassed, total.nran);
	_test_nodes_free(&_test_tests); // Free tests database
	return total.npassed == total.nran;
}

// Set mult part of benchmark to integer
static void _test_bench_mult1(_test_bench_t *bench, size_t iters) {
	bench->mult = 1;
	bench->iters = iters;
}

// Set mult part of benchmark to range struct
static void _test_bench_mult2(_test_bench_t *bench, _test_bench_range_t range) {
	bench->mult = range.mult;
	bench->iters = range.iters;
}

// Results of a benchmark
typedef struct _test_benchres {
	double user; // Time spent in the actual code
	double sys;  // Time spent in kernel
} _test_benchres_t;

// Run 1 benchmark
_test_benchres_t _test_run_bench(_test_bench_t *bench) {
	void *state = bench->statesz ? malloc(bench->statesz) : NULL;
	if (bench->setup) bench->setup(state); // Run setup code

	// Time the function bench->iters times
	struct rusage start, end;
	getrusage(RUSAGE_SELF, &start);
	for (uint64_t i = 0; i < bench->mult; i++) {
		for (uint64_t j = 0; j < bench->iters; j++) {
			bench->iter(state, j);
		}
	}
	getrusage(RUSAGE_SELF, &end);

	if (bench->cleanup) bench->cleanup(state); // Run cleanup code
	if (state) free(state);

	// Get seconds difference
	_test_benchres_t res = {
		.user = (double)(end.ru_utime.tv_sec - start.ru_utime.tv_sec),
		.sys = (double)(end.ru_stime.tv_sec - start.ru_stime.tv_sec)};

	// Add on microsecond difference
	res.user += (double)(end.ru_utime.tv_usec - start.ru_utime.tv_usec)
		/ (double)1000000.0;
	res.sys += (double)(end.ru_stime.tv_usec - start.ru_stime.tv_usec)
		/ (double)1000000.0;
	return res;
}

// Print time taken for benchmark in milliseconds or microseconds
static void _testptime(const char *msg, double secs) {
	_testp("%s", msg);
	if (secs >= 1.0 / 1000.0) _testp("% 8fms", secs * 1000.0);
	else _testp("% 8fus", secs * 1000000.0);
}

// Run all benchmarks
static void _test_run_benchmarks(void) {
	_testp("\nbenchmarks: \n");
	for (size_t i = 0; i < _test_benches.len; i++) {
		_test_group_t *group = _test_benches.groups + i;
		const char    *name = strcmp(group->name, "_") == 0 ? ""
								    : group->name;

		// Put newline between each group
		if (i > 0) _testp("\n");

		// Run every benchmark in the group
		_test_group_foreach(group, _test_bench_t, j) {
			// Skip empty benchmarks
			if (j->iter == NULL) {
				_testppadded(
					test_padding, "%s %s", name, j->name);
				_testp("[SKIP]\n");
				continue;
			}

			// Tell user we're in progress
			_testppadded(test_padding, "%s %s", name, j->name);
			_testp("...");

			// Run the benchmarks (devote all attention to it)
			_test_benchres_t res = _test_run_bench(j);

			// Print new results
			const size_t iters = j->mult * j->iters;
			_testp("\r");
			_testppadded(test_padding, "%s %s", name, j->name);
			_testp(" (%zu/%zu)    \n", iters, iters);
			_testp("overall:\t\t\t per iter:\n");
			_testptime("usr: ", res.user);
			_testptime("\t\t usr: ", res.user / (double)iters);
			_testp("\n");
			_testptime("sys: ", res.sys);
			_testptime("\t\t sys: ", res.sys / (double)iters);
			_testp("\n");

			fflush(stdout); // Make sure bench stdout is flushed
			fflush(stderr); // Make sure bench stderr is flushed
		}
	}
	_test_nodes_free(&_test_benches); // Free benchmark database
}

// Print optional flags to user
static int _test_args_usage(char **argv) {
	printf("usage:\n");
	printf("%s <options>\n", argv[0]);
	printf("\t-s,--silent\t\tdon't print test stdout\n");
	printf("\t-B,--no-bench\t\tdon't run the benchmarks\n");
	printf("\t-lfile\t\t\tredirect stdout to file\n");
	return 0;
}

// _test_realout		old stdout
// _test_stdout			read side of stdout pipe tests use
static void _test_redirect_stdout(void) {
	if (!_test_logfile) {
		_test_realout = stdout;
		return;
	}
	_test_realout = fdopen(dup(1), "w");
	freopen(_test_logfile, "w", stdout);
}

// Restore stdout and close previous pipes
static void _test_restore_stdout(void) {
	if (!_test_logfile) return;
	dup2(fileno(_test_realout), 1);
}

// Set main for user so that the tests file can be as simple as possible
int main(int argc, char **argv) {
	// Get arguments
	for (int i = 1; i < argc; i++) {
		_test_silent |= strcmp(argv[i], "-s") == 0
			| strcmp(argv[i], "--silent") == 0;
		_test_bench_disable |= strcmp(argv[i], "-B") == 0
			| strcmp(argv[i], "--no-bench") == 0;
		if (strncmp(argv[i], "-l", 2) == 0 && strlen(argv[i]) > 2) {
			_test_logfile = argv[i] + 2;
		}
		if (strcmp(argv[i], "-h") == 0
		    || strcmp(argv[i], "--help") == 0) {
			return _test_args_usage(argv);
		}
	}

	if (_test_silent) _test_logfile = "/dev/null";
	_test_redirect_stdout();
	bool tests_result = _test_run_tests();
	if (!_test_bench_disable) _test_run_benchmarks();
	_test_restore_stdout();

	return tests_result ? 0 : 1;
}
