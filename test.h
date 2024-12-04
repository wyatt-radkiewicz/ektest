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
#include <errno.h>

#ifndef test_padding
#define test_padding 32
#endif
#define _test_group(_group, _name) \
	static test_fn test_##_group##_##_name;\
	\
	static __attribute__((constructor(__COUNTER__ + 101))) \
		void _add_test_##_group##_##_name(void) \
	{ \
		_tests_add(#_group, (test_t){ \
			.fn = test_##_group##_##_name, \
			.name = #_name, \
		}); \
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

//#define bench()

typedef struct test_result {
	char msg[4096];
	bool pass;
} test_result_t;

typedef void (test_fn)(test_result_t *_out);

typedef struct test {
	test_fn *fn;
	const char *name;
} test_t;

typedef struct test_group {
	const char *name;
	test_t *tests;
	size_t len;
	size_t capacity;
} test_group_t;

typedef struct tests {
	test_group_t *groups;
	size_t len;
	size_t capacity;
} tests_t;

static tests_t _tests;
static FILE *_test_stdout, *_test_stderr, *_test_realout, *_test_realerr;
static bool _test_silent, _test_silent_err;

static void _tests_add(const char *name, test_t test) {
	test_group_t *group;

	for (size_t i = 0; i < _tests.len; i++) {
		if (strcmp(name, _tests.groups[i].name) == 0) {
			group = _tests.groups + i;
			goto add;
		}
	}
	
	if (!_tests.capacity) {
		// Initialize the tests collection
		_tests.capacity = 8;
		_tests.groups = malloc(sizeof(*_tests.groups)
				* _tests.capacity);
		_tests.len = 0;
	}

	// Add the group
	if (_tests.len >= _tests.capacity) {
		_tests.capacity *= 2;
		_tests.groups = realloc(_tests.groups,
			sizeof(*_tests.groups) * _tests.capacity);
	}
	_tests.groups[_tests.len] = (test_group_t){
		.name = name,
		.len = 0,
		.capacity = 8,
		.tests = malloc(sizeof(test_t) * 8),
	};
	group = _tests.groups + _tests.len++;

add:
	if (group->len >= group->capacity) {
		group->capacity *= 2;
		group->tests = realloc(group->tests,
			sizeof(*group->tests) * group->capacity);
	}
	group->tests[group->len++] = test;
}

static void _tests_free(void) {
	for (size_t i = 0; i < _tests.len; i++) {
		free(_tests.groups[i].tests);
	}
	free(_tests.groups);
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

static bool _run_tests(void) {
	struct { size_t npassed, nran; } total = {0};

	_redirect_stdout();

	for (size_t i = 0; i < _tests.len; i++) {
		test_group_t *group = _tests.groups + i;
		const char *name = strcmp(group->name, "_") == 0
			? "" : group->name;
		size_t npassed = 0;

		tprintf("\n");
		if (name[0] == '\0') {
			tprintf("%zu tests\n", group->len);
		} else {
			tprintf("%s (%zu tests)\n", name, group->len);
		}

		for (size_t j = 0; j < group->len; j++) {
			npassed += _run_test(group->tests + j);
		}

		total.npassed += npassed, total.nran += group->len;
		_print_results(name, npassed, group->len);
	}

	tprintf("\n\n");
	_print_results("test", total.npassed, total.nran);
	_tests_free();
	fclose(stdout);
	fclose(_test_stdout);
	fclose(_test_realout);
	fclose(stderr);
	fclose(_test_stderr);
	fclose(_test_realerr);
	return total.npassed == total.nran;
}

int main(int argc, char **argv) {
	for (int i = 1; i < argc; i++) {
		_test_silent |= strcmp(argv[i], "-s") == 0;
		_test_silent |= _test_silent_err |= strcmp(argv[i], "-se") == 0;
	}

	return _run_tests() ? 0 : 1;
}

