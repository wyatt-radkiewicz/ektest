#pragma once

// Include everything the user might need
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>

#ifndef test_padding
#define test_padding 32
#endif
#define _test_group(_group, _name) \
	static test_fn test_##_group##_##_name; \
	\
	static __attribute__((constructor(__COUNTER__ + 101))) \
		void _add_test_##_group##_##_name(void) \
	{ \
		_nodes_add(&_tests, #_group, &(test_t){ \
			.fn = test_##_group##_##_name, \
			.name = #_name, \
		}, sizeof(test_t)); \
	} \
	static void test_##_group##_##_name(test_result_t *_out)
#define _test(_name) _test_group(_, _name)
#define _pick_test(_1, _0, _name, ...) _name
#define test(...) _pick_test(__VA_ARGS__, _test_group, _test)(__VA_ARGS__)

#define pass() return
#define log(msg, __VA_ARGS__) (printf("%s [line %d]: " msg, \
			__func__, __LINE__, ##_VA_ARGS__))
#define _fail_info "[FAIL (line %d)]\x1B[0m"
#define _fail_msg _fail_info "\n\x1B[30;41mMESSAGE:\x1B[0m "
#define _skip_first(_first, ...) , ##__VA_ARGS__
#define fail(...) do { \
		const char *_msg; \
		(void)(_msg = _fail_msg __VA_ARGS__); \
		if (strcmp(_msg, _fail_msg) == 0 || _test_silent) { \
			_msg = _fail_info; \
		} \
		snprintf(_out->msg, sizeof(_out->msg), \
			_msg, __LINE__ _skip_first(__VA_ARGS__)); \
		_out->pass = false; \
		return; \
	} while (false)
#define tprintf(...) fprintf(_test_realout, __VA_ARGS__)
#define teprintf(...) fprintf(_test_realerr, __VA_ARGS__)

#define _bench_group(_group, _name, _iters) \
	static __attribute__((constructor(__COUNTER__ + 101))) \
		void _add_bench_##_group##_##_name(void) { \
		bench_t *bench = _nodes_add(&_benches, #_group, &(bench_t){ \
			._setup = NULL, \
			._cleanup = NULL, \
			._iter = NULL, \
			.name = #_name, \
			.iters = _iters, \
		}, sizeof(bench_t)); \
		_bench_setup = &bench->_setup; \
		_bench_cleanup = &bench->_cleanup; \
		_bench_iter = &bench->_iter; \
	}
#define _bench(_name, _iters) _bench_group(_, _name, _iters)
#define _pick_bench(_2, _1, _0, _name, ...) _name
#define bench(...) _pick_bench(__VA_ARGS__, _bench_group, _bench)(__VA_ARGS__)
#define _test_concat_(_1, _2) _1##_2
#define _test_concat(_1, _2) _test_concat_(_1, _2)
#define _append_line(_name) _test_concat(_name, __LINE__)
#define onsetup \
	static void _append_line(_bench_setup)(void); \
	static __attribute__((constructor(__COUNTER__ + 101))) \
		void _append_line(_bench_set_setup)(void) { \
		*_bench_setup = _append_line(_bench_setup);\
	} \
	static void _append_line(_bench_setup)(void)
#define oncleanup \
	static void _append_line(_bench_cleanup)(void); \
	static __attribute__((constructor(__COUNTER__ + 101))) \
		void _append_line(_bench_set_cleanup)(void) { \
		*_bench_cleanup = _append_line(_bench_cleanup);\
	} \
	static void _append_line(_bench_cleanup)(void)
#define oniter \
	static void _append_line(_bench_iter)(void); \
	static __attribute__((constructor(__COUNTER__ + 101))) \
		void _append_line(_bench_set_iter)(void) { \
		*_bench_iter = _append_line(_bench_iter);\
	} \
	static void _append_line(_bench_iter)(void)

typedef struct test_result {
	char msg[4096];
	bool pass;
} test_result_t;

typedef void (test_fn)(test_result_t *_out);

typedef struct test {
	test_fn *fn;
	const char *name;
} test_t;

typedef struct group {
	const char *name;
	void *nodes;
	size_t len, capacity;
} group_t;

#define group_foreach(_group, _type, _name) \
	for (_type *_name = (_group)->nodes; \
		_name != ((_type *)(_group)->nodes) + (_group)->len; \
		_name++)

typedef struct nodes {
	group_t *groups;
	size_t len;
	size_t nodesz;
	size_t capacity;
} nodes_t;

typedef void (bench_fn)(void);

typedef struct bench {
	const char *name;
	bench_fn *_setup, *_cleanup, *_iter;
	size_t iters;
} bench_t;

static nodes_t _tests, _benches;
static bench_fn **_bench_setup;
static bench_fn **_bench_cleanup;
static bench_fn **_bench_iter;
static FILE *_test_stdout, *_test_stderr, *_test_realout, *_test_realerr;
static bool _test_silent, _test_silent_err, _bench_disable;

static void *_nodes_add(nodes_t *nodes, const char *name,
			void *node, size_t nodesz) {
	group_t *group;

	for (size_t i = 0; i < nodes->len; i++) {
		if (strcmp(name, nodes->groups[i].name) == 0) {
			group = nodes->groups + i;
			goto add;
		}
	}
	
	if (!nodes->capacity) {
		// Initialize the tests collection
		*nodes = (nodes_t){
			.capacity = 8,
			.groups = malloc(sizeof(*nodes->groups) * 8),
			.nodesz = nodesz,
			.len = 0,
		};
	}

	// Add the group
	if (nodes->len >= nodes->capacity) {
		nodes->capacity *= 2;
		nodes->groups = realloc(nodes->groups,
			sizeof(*nodes->groups) * nodes->capacity);
	}
	nodes->groups[nodes->len] = (group_t){
		.name = name,
		.len = 0,
		.capacity = 8,
		.nodes = malloc(nodes->nodesz * 8),
	};
	group = nodes->groups + nodes->len++;

add:
	if (group->len >= group->capacity) {
		group->capacity *= 2;
		group->nodes = realloc(group->nodes,
			nodes->nodesz * group->capacity);
	}
	memcpy((void *)((uintptr_t)group->nodes + group->len * nodes->nodesz),
		node, nodesz);
	return (void *)((uintptr_t)group->nodes + group->len++ * nodes->nodesz);
}

static void _nodes_free(nodes_t *nodes) {
	for (size_t i = 0; i < nodes->len; i++) {
		free(nodes->groups[i].nodes);
	}
	free(nodes->groups);
}

static void _print_padded(int to, const char *str, ...) {
	va_list args;
	va_start(args, str);
	to -= vfprintf(_test_realout, str, args) - 1;
	for (;to > 0;to--) putc(' ', _test_realout);
	va_end(args);
}

static bool _run_test(const test_t *test) {
	_print_padded(test_padding, "%s:", test->name);
	tprintf(" RUNNING...");
	
	static test_result_t result;
	result.msg[0] = '\0';
	result.pass = true;
	test->fn(&result);
	fflush(stdout);

	tprintf("\r");
	_print_padded(test_padding, "%s:", test->name);
	if (result.pass) tprintf(" \x1B[37;42m[PASS]\x1B[0m       \n");
	else tprintf(" \x1B[30;41m%s       \n", result.msg);

	static char outbuf[4096];
	size_t outsz = fread(outbuf, 1, sizeof(outbuf) - 1, _test_stdout);
	if (outsz == 0 || _test_silent) goto check_stderr;
	tprintf("STDOUT:\n");
	do {
		outbuf[outsz] = '\0';
		tprintf("%s", outbuf);
	} while ((outsz = fread(outbuf, 1, sizeof(outbuf) - 1, _test_stdout)));
	
check_stderr:
	outsz = fread(outbuf, 1, sizeof(outbuf) - 1, _test_stderr);
	if (outsz == 0 || _test_silent_err) return result.pass;
	fflush(_test_realout);
	teprintf("\x1B[30;41mSTDERR\x1B[0m:\n");
	do {
		outbuf[outsz] = '\0';
		teprintf("%s", outbuf);
	} while ((outsz = fread(outbuf, 1, sizeof(outbuf) - 1, _test_stderr)));
	fflush(_test_realerr);

	return result.pass;
}

static void _print_results(const char *name, size_t passed, size_t ran) {
	if (name[0] == '\0') tprintf("results");
	else tprintf("%s results", name);
	tprintf(" (%zu test%s)\n", ran, passed == 1 ? "" : "s");
	tprintf("%zu/%zu test%spassed (%zu%%)\n", passed, ran,
		passed == 1 ? " " : "s ", passed * 100 / ran);
}

static void _redirect_stdout(void) {
	int pipe_fd[2];

	_test_realout = fdopen(dup(STDOUT_FILENO), "w");
	pipe(pipe_fd);
	fcntl(pipe_fd[0], F_SETFL, O_NONBLOCK);
	dup2(pipe_fd[1], STDOUT_FILENO);
	_test_stdout = fdopen(pipe_fd[0], "r");

	_test_realerr = fdopen(dup(STDERR_FILENO), "w");
	pipe(pipe_fd);
	fcntl(pipe_fd[0], F_SETFL, O_NONBLOCK);
	dup2(pipe_fd[1], STDERR_FILENO);
	_test_stderr = fdopen(pipe_fd[0], "r");
}

static void _restore_stdout(void) {
	dup2(fileno(_test_realout), STDOUT_FILENO);
	fclose(_test_stdout);
	fclose(_test_realout);
	dup2(fileno(_test_realerr), STDERR_FILENO);
	fclose(_test_stderr);
	fclose(_test_realerr);
}

static void _run_test_group(group_t *group, bool run_unnamed,
			size_t *total_passed, size_t *total_ran) {
	const char *name = strcmp(group->name, "_") == 0
		? "" : group->name;
	size_t npassed = 0;

	if (run_unnamed && name[0] != '\0') return;
	if (!run_unnamed && name[0] == '\0') return;
	if (!run_unnamed) {
		tprintf("%s (%zu test%s)\n", name, group->len,
			group->len > 1 ? "s" : "");
	}

	group_foreach(group, test_t, j) {
		npassed += _run_test(j);
	}

	*total_passed += npassed, *total_ran += group->len;
	_print_results(name, npassed, group->len);
}

static bool _run_tests(void) {
	struct { size_t npassed, nran; } total = {0};

	_redirect_stdout();
	tprintf("tests:\n");
	for (size_t i = 1; i < 2; i--) {
		for (size_t j = 0; j < _tests.len; j++) {
			_run_test_group(_tests.groups + j, i,
					&total.npassed, &total.nran);
		}
		tprintf("\n");
	}

	if (_tests.len > 1) _print_results("test", total.npassed, total.nran);
	_nodes_free(&_tests);
	return total.npassed == total.nran;
}

typedef struct bench_results {
	double user, sys;
} bench_results_t;

bench_results_t _run_bench(bench_t *bench) {
	if (bench->_setup) bench->_setup();
	struct rusage start, end;
	getrusage(RUSAGE_SELF, &start);
	for (size_t i = 0; i < bench->iters; i++) bench->_iter();
	getrusage(RUSAGE_SELF, &end);
	if (bench->_cleanup) bench->_cleanup();
	bench_results_t res = {
		.user = (double)(end.ru_utime.tv_sec - start.ru_utime.tv_sec),
		.sys = (double)(end.ru_stime.tv_sec - start.ru_stime.tv_sec)
	};
	res.user += (double)(end.ru_utime.tv_usec - start.ru_utime.tv_usec)
		/ (double)1000000.0;
	res.sys += (double)(end.ru_stime.tv_usec - start.ru_stime.tv_usec)
		/ (double)1000000.0;
	return res;
}

static void _print_time(const char *msg, double secs) {
	tprintf("%s", msg);
	if (secs >= 1000.0) tprintf("% 8fms", secs * 1000.0);
	else tprintf("% 8fus", secs * 1000000.0);
}

static void _run_benches(void) {
	tprintf("\nbenchmarks: \n");
	for (size_t i = 0; i < _benches.len; i++) {
		group_t *group = _benches.groups + i;
		const char *name = strcmp(group->name, "_") == 0
			? "" : group->name;

		if (i > 0) tprintf("\n");
		group_foreach(group, bench_t, j) {
			_print_padded(32, "%s %s", group->name, j->name);
			tprintf("...");

			bench_results_t res = _run_bench(j);

			tprintf("\r");
			_print_padded(32, "%s %s", group->name, j->name);
			tprintf(" (%zu/%zu)    \n", j->iters, j->iters);
			tprintf("overall:\t\t\tper iter:\n");
			_print_time("usr: ", res.user);
			_print_time("\t\tusr: ", res.user / (double)j->iters);
			tprintf("\n");
			_print_time("sys: ", res.sys);
			_print_time("\t\tsys: ", res.sys / (double)j->iters);
			tprintf("\n");
		}
	}

	_nodes_free(&_benches);
}

static int _test_args_usage(char **argv) {
	printf("usage:\n");
	printf("%s options\n", argv[0]);
	printf("\t-s,--silent\t\tdon't print test stdout\n");
	printf("\t-S,--silent-errors\tdon't print test stdout or stderr\n");
	printf("\t-B,--no-bench\t\tdon't run the benchmarks\n");
	return 0;
}

int main(int argc, char **argv) {
	for (int i = 1; i < argc; i++) {
		_test_silent |= strcmp(argv[i], "-s") == 0
			| strcmp(argv[i], "--silent") == 0;
		_test_silent |= _test_silent_err
			|= strcmp(argv[i], "-S") == 0
			| strcmp(argv[i], "-silence-errors") == 0;
		_bench_disable |= strcmp(argv[i], "-B") == 0
			| strcmp(argv[i], "--no-bench") == 0;
		if (strcmp(argv[i], "-h") == 0
			|| strcmp(argv[i], "--help") == 0) {
			return _test_args_usage(argv);
		}
	}

	bool tests_result = _run_tests();
	if (!_bench_disable) _run_benches();
	_restore_stdout();
	return tests_result ? 0 : 1;
}

