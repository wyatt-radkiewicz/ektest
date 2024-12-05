#pragma once

// Include everything the user might need (and we need)
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>

// Used to redirect stdout so tests don't mess up the test output
#include <unistd.h>
#include <fcntl.h>

// Used to get user and system time from benchmarks
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>

// Default column of test results
#ifndef test_padding
#define test_padding 32
#endif

// Macro to make a function run automatically
// Use the ctor function as a stub to just add the real function.
// Use the ctor priorty to enforce run order (0-100 reserved)
#define _test_autorun __attribute__((constructor(__COUNTER__ + 101)))

// Creates a test in _group with _name
#define _test_group(_group, _name) \
	/* Forward declare the test so we can add its function pointer */ \
	static _test_fn _test_##_group##_##_name; \
	static _test_autorun void _test_add_##_group##_##_name(void) { \
		_test_nodes_add(&_test_tests, #_group, &(_test_t){ \
			.fn = _test_##_group##_##_name, \
			.name = #_name, \
		}, sizeof(_test_t)); \
	} \
	/* The actual code of the test.
	 * _out already is setup for a passing test. */ \
	static uint8_t *_test_##_group##_##_name(void)

// Macros to allow the user to just pass in test name or also include group
#define _test(_name) _test_group(_, _name)
#define _test_pick(_1, _0, _name, ...) _name
#define test(...) _test_pick(__VA_ARGS__, _test_group, _test)(__VA_ARGS__)

// Can be used in test or benchmark functions
#define log(msg, ...) (printf("[line %d]: " msg, __LINE__, ##__VA_ARGS__))

// Pass and fail macros
#define pass return NULL
#define fail return __LINE__ + (uint8_t *)_test_fail

// Used in this library inplace of normal printf and printing to stderr
#define _testp(...) fprintf(_test_realout, __VA_ARGS__)
#define _testep(...) fprintf(_test_realerr, __VA_ARGS__)

// Internal macro to create a benchmark in _group with _name and n _iters
#define _test_bench_group(_group, _name, _iters) \
	static _test_autorun void _test_bench_add_##_group##_##_name(void) { \
		_test_bench_t *bench = _test_nodes_add(&_test_benches, \
			#_group, \
			&(_test_bench_t){ \
				.setup = NULL, \
				.cleanup = NULL, \
				.iter = NULL, \
				.name = #_name, \
				.iters = _iters, \
			}, sizeof(_test_bench_t) \
		); \
		/* To my knowledge you can only use unique names in macros
		 * in the macro itself but not in other macros, so here we
		 * programmatically save the function pointers for when the
		 * user calls the other macros to set the func. pointers */ \
		_test_bench_setup = &bench->setup; \
		_test_bench_cleanup = &bench->cleanup; \
		_test_bench_iter = &bench->iter; \
	}

// Use the bench macro and pick default group argument if its left out
#define _test_bench(_name, _iters) _test_bench_group(_, _name, _iters)
#define _test_bench_pick(_2, _1, _0, _name, ...) _name
#define bench(...) _test_bench_pick(__VA_ARGS__, _test_bench_group, \
		_bench)(__VA_ARGS__)

// Normal concatination macros to make sure arguments are expanded
#define _test_concat2(_1, _2) _1##_2
#define _test_concat(_1, _2) _test_concat2(_1, _2)
#define _test_appendline(_name) _test_concat(_name, __LINE__)

// Functions used to set benchmark functions (setup, cleanup, iter)
#define onsetup \
	static _test_bench_setup_fn _test_appendline(_test_bench_setup); \
	static _test_autorun void _test_appendline(_test_bench_sets)(void) { \
		*_test_bench_setup = _test_appendline(_test_bench_setup);\
	} \
	static void _test_appendline(_test_bench_setup)(void)
#define oncleanup \
	static _test_bench_cleanup_fn _test_appendline(_test_bench_cleanup); \
	static _test_autorun void _test_appendline(_test_bench_sets)(void) { \
		*_test_bench_cleanup = _test_appendline(_test_bench_cleanup);\
	} \
	static void _test_appendline(_test_bench_cleanup)(void)
#define oniter \
	static _test_bench_iter_fn _test_appendline(_test_bench_iter); \
	static _test_autorun void _test_appendline(_test_bench_sets)(void) { \
		*_test_bench_iter = _test_appendline(_test_bench_iter);\
	} \
	static void _test_appendline(_test_bench_iter)(void)

// Test function prototype.
// Returns NULL if test succeeded.
typedef uint8_t *(_test_fn)(void);

// This function gets run (or address returned) if the test failed
static void *_test_fail(const char *fmt, ...);

// Structure used to defined tests
typedef struct _test {
	_test_fn *fn;
	const char *name; // Name of the test without "test" prefix
} _test_t;

// Benchmarks and tests are grouped so this structure is just here to store
// either of them
typedef struct _test_group {
	const char *name;
	void *nodes;
	size_t len, capacity;
} _test_group_t;

// Loops through each node in _group with _name (pointer to _type)
#define _test_group_foreach(_group, _type, _name) \
	for (_type *_name = (_group)->nodes; \
		_name != ((_type *)(_group)->nodes) + (_group)->len; \
		_name++)

// A database of groups of something to run (tests, benchmarks)
typedef struct _test_nodes {
	_test_group_t *groups;
	size_t len;
	size_t nodesz;
	size_t capacity;
} _test_nodes_t;

// Benchmark function prototypes
typedef void (_test_bench_setup_fn)(void);
typedef void (_test_bench_cleanup_fn)(void);
typedef void (_test_bench_iter_fn)(void);

// Structure used to defined benchmarks
typedef struct _test_bench {
	const char *name;
	_test_bench_setup_fn *setup;
	_test_bench_cleanup_fn *cleanup;
	_test_bench_iter_fn *iter;
	size_t iters;		// How many times iter should be called
} _test_bench_t;

// Database of tests and benchmarks that is built up with the ctor functions
static _test_nodes_t _test_tests, _test_benches;

// Used in benchmark macros to set the current benchmark's callbacks
static _test_bench_setup_fn **_test_bench_setup;
static _test_bench_cleanup_fn **_test_bench_cleanup;
static _test_bench_iter_fn **_test_bench_iter;

// Used to globally redirect stdout and stderr
static FILE *_test_stdout, *_test_stderr, *_test_realout, *_test_realerr;

// Command line flags
static bool _test_silent;	// Don't print test's stdout
static bool _test_silent_err;	// Don't print any of test's output
static bool _test_bench_disable;// Don't run benchmarks

// Add a node (test, benchmark) to a nodes database
static void *_test_nodes_add(_test_nodes_t *nodes, const char *name,
			void *node, size_t nodesz) {
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
		nodes->groups = realloc(nodes->groups,
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
		group->nodes = realloc(group->nodes,
			nodes->nodesz * group->capacity);
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
	to -= vfprintf(_test_realout, str, args) - 1;
	for (;to > 0;to--) putc(' ', _test_realout);
	va_end(args);
}

// Run a test and return if it succeeded.
static bool _test_run_test(const _test_t *test) {
	_testppadded(test_padding, "%s:", test->name);
	_testp(" ...");
	
	// Run the test
	uint8_t *res = test->fn();
	size_t failline = (uintptr_t)res - (uintptr_t)_test_fail;
	bool passed = res == NULL;
	fflush(stdout);	// Make sure what the test printed is flushed

	// Print new status
	_testp("\r");
	_testppadded(test_padding, "%s:", test->name);
	if (passed) _testp(" \x1B[37;42m[PASS]\x1B[0m\n");
	else _testp(" \x1B[30;41m[FAIL (line %zu)]\x1B[0m\n", failline);

	// See what the test printed to stdout
	static char outbuf[4096];
	size_t outsz = fread(outbuf, 1, sizeof(outbuf) - 1, _test_stdout);
	if (outsz == 0 || _test_silent) goto check_stderr; // It printed nothing
	_testp("STDOUT: "); // Start printing whatever it printed
	do {
		outbuf[outsz] = '\0';
		_testp("%s", outbuf);
	} while ((outsz = fread(outbuf, 1, sizeof(outbuf) - 1, _test_stdout)));
	_testp("\n");
	
check_stderr:
	// Likewise but with stderr
	outsz = fread(outbuf, 1, sizeof(outbuf) - 1, _test_stderr);
	if (outsz == 0 || _test_silent_err) return passed;
	fflush(_test_realout);
	_testep("\x1B[30;41mSTDERR\x1B[0m: ");
	do {
		outbuf[outsz] = '\0';
		_testep("%s", outbuf);
	} while ((outsz = fread(outbuf, 1, sizeof(outbuf) - 1, _test_stderr)));
	fflush(_test_realerr);
	_testp("\n");

	return passed;
}

// Fails a currently running test
static void *_test_fail(const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
	return _test_fail;
}

// Print pretty results of test suite
static void _testpres(const char *name, size_t passed, size_t ran) {
	if (name[0] == '\0') _testp("results");
	else _testp("%s results", name);
	_testp(" (%zu test%s)\n", ran, passed == 1 ? "" : "s");
	_testp("%zu/%zu test%spassed (%zu%%)\n", passed, ran,
		passed == 1 ? " " : "s ", passed * 100 / ran);
}

// Redirect stdout to a pipe.
// _test_realout		old stdout
// _test_stdout			read side of stdout pipe tests use
// _test_realerr		old stderr
// _test_stderr			read side of stderr pipe tests use
static void _test_redirect_stdout(void) {
	int pipe_fd[2];

	// Create copy of current stdout so we can use it
	_test_realout = fdopen(dup(STDOUT_FILENO), "w");

	// Create pipe (pipe_fd[0] is read side, pipe_fd[1] is write side)
	pipe(pipe_fd);

	// Set to non block so that if a test didn't print anything we're ok
	fcntl(pipe_fd[0], F_SETFL, O_NONBLOCK);

	// Set normal stdout to write side of our pipe
	dup2(pipe_fd[1], STDOUT_FILENO);

	// Make it so that we can read what tests write out
	_test_stdout = fdopen(pipe_fd[0], "r");

	// Do the same with stderr
	_test_realerr = fdopen(dup(STDERR_FILENO), "w");
	pipe(pipe_fd);
	fcntl(pipe_fd[0], F_SETFL, O_NONBLOCK);
	dup2(pipe_fd[1], STDERR_FILENO);
	_test_stderr = fdopen(pipe_fd[0], "r");
}

// Restore stdout and close previous pipes
static void _test_restore_stdout(void) {
	// Set stdout to the real one now
	dup2(fileno(_test_realout), STDOUT_FILENO);
	fclose(_test_stdout);		// Close the pipe
	fclose(_test_realout);
	dup2(fileno(_test_realerr), STDERR_FILENO);
	fclose(_test_stderr);
	fclose(_test_realerr);
}

// Run a group of tests.
static void _test_run_test_group(_test_group_t *group, bool run_unnamed,
			size_t *total_passed, size_t *total_ran) {
	const char *name = strcmp(group->name, "_") == 0
		? "" : group->name;
	size_t npassed = 0;

	if (run_unnamed && name[0] != '\0') return;
	if (!run_unnamed && name[0] == '\0') return;
	if (!run_unnamed) {
		_testp("%s (%zu test%s)\n", name, group->len,
			group->len > 1 ? "s" : "");
	}

	_test_group_foreach(group, _test_t, j) {
		npassed += _test_run_test(j);
	}

	*total_passed += npassed, *total_ran += group->len;
	_testpres(name, npassed, group->len);
}

// Run all tests
static bool _test_run_tests(void) {
	struct { size_t npassed, nran; } total = {0};

	// Try and run unnamed group first and if not, then run other groups
	_testp("tests:\n");
	for (size_t i = 1; i < 2; i--) {
		for (size_t j = 0; j < _test_tests.len; j++) {
			_test_run_test_group(_test_tests.groups + j, i,
						&total.npassed, &total.nran);
		}
		_testp("\n");
	}

	// Overall results
	if (_test_tests.len > 1) _testpres("test", total.npassed, total.nran);
	_test_nodes_free(&_test_tests);	// Free tests database
	return total.npassed == total.nran;
}

// Results of a benchmark
typedef struct _test_benchres {
	double user;	// Time spent in the actual code
	double sys;	// Time spent in kernel
} _test_benchres_t;

// Run 1 benchmark
_test_benchres_t _test_run_bench(_test_bench_t *bench) {
	if (bench->setup) bench->setup();	// Run setup code
	
	// Time the function bench->iters times
	struct rusage start, end;
	getrusage(RUSAGE_SELF, &start);
	for (size_t i = 0; i < bench->iters; i++) bench->iter();
	getrusage(RUSAGE_SELF, &end);

	if (bench->cleanup) bench->cleanup();	// Run cleanup code

	// Get seconds difference
	_test_benchres_t res = {
		.user = (double)(end.ru_utime.tv_sec - start.ru_utime.tv_sec),
		.sys = (double)(end.ru_stime.tv_sec - start.ru_stime.tv_sec)
	};

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
	if (secs >= 1000.0) _testp("% 8fms", secs * 1000.0);
	else _testp("% 8fus", secs * 1000000.0);
}

// Run all benchmarks
static void _test_run_benchmarks(void) {
	_testp("\nbenchmarks: \n");
	for (size_t i = 0; i < _test_benches.len; i++) {
		_test_group_t *group = _test_benches.groups + i;
		const char *name = strcmp(group->name, "_") == 0
			? "" : group->name;

		// Put newline between each group
		if (i > 0) _testp("\n");

		// Run every benchmark in the group
		_test_group_foreach(group, _test_bench_t, j) {
			// Tell user we're in progress
			_testppadded(32, "%s %s", group->name, j->name);
			_testp("...");

			// Run the benchmarks (devote all attention to it)
			_test_benchres_t res = _test_run_bench(j);

			// Print new results
			_testp("\r");
			_testppadded(32, "%s %s", group->name, j->name);
			_testp(" (%zu/%zu)    \n", j->iters, j->iters);
			_testp("overall:\t\t\tper iter:\n");
			_testptime("usr: ", res.user);
			_testptime("\t\tusr: ", res.user / (double)j->iters);
			_testp("\n");
			_testptime("sys: ", res.sys);
			_testptime("\t\tsys: ", res.sys / (double)j->iters);
			_testp("\n");
		}
	}
	_test_nodes_free(&_test_benches); // Free benchmark database
}

// Print optional flags to user
static int _test_args_usage(char **argv) {
	printf("usage:\n");
	printf("%s <options>\n", argv[0]);
	printf("\t-s,--silent\t\tdon't print test stdout\n");
	printf("\t-S,--silent-errors\tdon't print test stdout or stderr\n");
	printf("\t-B,--no-bench\t\tdon't run the benchmarks\n");
	return 0;
}

// Set main for user so that the tests file can be as simple as possible
int main(int argc, char **argv) {
	// Get arguments
	for (int i = 1; i < argc; i++) {
		_test_silent |= strcmp(argv[i], "-s") == 0
			| strcmp(argv[i], "--silent") == 0;
		_test_silent |= _test_silent_err
			|= strcmp(argv[i], "-S") == 0
			| strcmp(argv[i], "-silence-errors") == 0;
		_test_bench_disable |= strcmp(argv[i], "-B") == 0
			| strcmp(argv[i], "--no-bench") == 0;
		if (strcmp(argv[i], "-h") == 0
			|| strcmp(argv[i], "--help") == 0) {
			return _test_args_usage(argv);
		}
	}

	_test_redirect_stdout();
	bool tests_result = _test_run_tests();
	if (!_test_bench_disable) _test_run_benchmarks();
	_test_restore_stdout();

	return tests_result ? 0 : 1;
}

