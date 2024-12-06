#pragma once
// version 2.0.0
// Examples:
// #include "test.h"
// 
// test(always_passes) {
// 	pass;
// }
// test(always_fails) {
// 	fail;
// }
// test(asserts_always) {
// 	assert(5 == 4);
// 	pass;
// }
// test(asserts_message) {
// 	assert(5 == 4, "Wow this was dumb.");
// 	pass;
// }
// 
// const volatile char *strings[] = {
// 	"0.5",
// 	"0.6",
// 	"0.7",
// 	"0.8",
// };
// 
// bench_on(strtod1, strings, 10000) {
// 	// Use the built-in i parameter to get the current thing you are
// 	// looping over
// 	strtod((const char *)*i, NULL);
// }
// bench_for(strtod2, 10000) {
// 	strtod("0.4", NULL);
// }
//

#define _test_concat1(_0, _1) _0##_1
#define _test_concat(_0, _1) _test_concat1(_0, _1)

#define test(_name)                                                            \
	static int test_##_name(void);                                         \
	_test_t _test_concat(_test, __COUNTER__) = {                           \
		.testfn = test_##_name, .istest = true, .name = #_name};       \
	static int test_##_name(void)

#define bench_for(_name, _times)                                               \
	static void bench_##_name(void *);                                     \
	_test_t _test_concat(_test, __COUNTER__) = {                           \
		.bench =                                                       \
			{                                                      \
				.fn = bench_##_name,                           \
				.iters = _times,                               \
				.nitems = 1,                                   \
				},                                                     \
		.name = #_name \
	      };                                               \
	static void bench_##_name(void *_______)
#define bench_on(_name, _array, _times)                                        \
	static void bench_##_name(typeof((_array)[0]) *const);                 \
	_test_t _test_concat(_test, __COUNTER__) = {                           \
		.bench =                                                       \
			{                                                      \
				.fn = (_test_bench_fn *)bench_##_name,         \
				.array = _array,                               \
				.step = sizeof((_array)[0]),                   \
				.nitems =                                      \
					sizeof(_array) / sizeof((_array)[0]),  \
				.iters = _times,                               \
				},                                                     \
		.name = #_name \
	      };                                               \
	static void bench_##_name(typeof((_array)[0]) *const i)

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <stdlib.h>

#define pass return 0
#define fail return __LINE__
#define assert(_cond, ...)                                                     \
	if (!(_cond)) {                                                        \
		printf(" Assertion failed: " __VA_ARGS__);                     \
		printf("\n");                                                  \
		fail;                                                          \
	}

typedef int(_test_fn)(void);
typedef void(_test_bench_fn)(void *);

typedef struct _test_bench {
	_test_bench_fn *fn;
	void *array;
	size_t step, nitems, iters;
} _test_bench_t;

typedef struct _test {
	union {
		_test_fn *testfn;
		_test_bench_t bench;
	};
	const char *name;
	bool istest;
} _test_t;

typedef struct _test_state {
	_test_t *benches[1024];
	int passed, ran, nbenches;
} _test_state_t;

static void _test_run_bench(const char *name, _test_bench_t *bench) {
	clock_t start = clock();
	const uintptr_t array_end =
		(uintptr_t)bench->array + bench->nitems * bench->step;
	for (size_t i = 0; i < bench->iters; i++) {
		for (uintptr_t addr = (uintptr_t)bench->array; addr < array_end;
		     addr += bench->step) {
			bench->fn((void *)addr);
		}
	}
	clock_t end = clock();

	printf("\x1B[34m[RAN]\x1B[0m %s (%-8zd iters) in ",
	       name,
	       bench->iters * bench->nitems);

	double secs = ((double)end - (double)start) / (double)CLOCKS_PER_SEC;
	if (secs < 1.0 / 1000.0) printf("%6.2fus ", secs * 1000000.0);
	else printf("%6.2fms ", secs * 1000.0);

	secs /= (double)(bench->iters * bench->nitems);
	printf("(");
	if (secs < 1.0 / 1000.0) printf("%6.2fus ", secs * 1000000.0);
	else printf("%6.2fms ", secs * 1000.0);
	printf("per iter)\n");
}

static bool _test_run(_test_t *test, _test_state_t *state) {
	int fail_line = test->testfn();
	printf("%s\x1B[0m %s",
	       !fail_line ? "\x1B[32m[PASS]" : "\x1B[31m[FAIL]",
	       test->name);
	if (fail_line) printf(" (on line %d)", fail_line);
	printf("\n");
	state->passed += !fail_line, state->ran++;
	return !fail_line;
}

static void _test_start(_test_state_t *state) {
	memset(state, 0, sizeof(*state));
}

static int _test_end(_test_state_t *state) {
	printf("%d/%d tests passed\n", state->passed, state->ran);

	// Run benchmarks if there is any
	if (state->nbenches) printf("\n");
	for (int i = 0; i < state->nbenches; i++) {
		_test_run_bench(
			state->benches[i]->name, &state->benches[i]->bench);
	}

	return state->passed == state->ran ? 0 : 1;
}

#define _test_tryrun(_test)                                                    \
	if (_test.istest) {                                                    \
		_test_run(&_test, state);                                      \
	} else {                                                               \
		if (!_test.bench.fn) return;                                   \
		state->benches[state->nbenches++] = &_test;                    \
	}

static void _tests_run_tests(_test_state_t *state);

int main(int argc, char **argv) {
	_test_state_t state;
	_test_start(&state);
	_tests_run_tests(&state);
	return _test_end(&state);
}

_test_t _test0, _test1, _test2, _test3, _test4, _test5, _test6, _test7, _test8,
	_test9, _test10, _test11, _test12, _test13, _test14, _test15, _test16,
	_test17, _test18, _test19, _test20, _test21, _test22, _test23, _test24,
	_test25, _test26, _test27, _test28, _test29, _test30, _test31, _test32,
	_test33, _test34, _test35, _test36, _test37, _test38, _test39, _test40,
	_test41, _test42, _test43, _test44, _test45, _test46, _test47, _test48,
	_test49, _test50, _test51, _test52, _test53, _test54, _test55, _test56,
	_test57, _test58, _test59, _test60, _test61, _test62, _test63, _test64,
	_test65, _test66, _test67, _test68, _test69, _test70, _test71, _test72,
	_test73, _test74, _test75, _test76, _test77, _test78, _test79, _test80,
	_test81, _test82, _test83, _test84, _test85, _test86, _test87, _test88,
	_test89, _test90, _test91, _test92, _test93, _test94, _test95, _test96,
	_test97, _test98, _test99, _test100, _test101, _test102, _test103,
	_test104, _test105, _test106, _test107, _test108, _test109, _test110,
	_test111, _test112, _test113, _test114, _test115, _test116, _test117,
	_test118, _test119, _test120, _test121, _test122, _test123, _test124,
	_test125, _test126, _test127, _test128, _test129, _test130, _test131,
	_test132, _test133, _test134, _test135, _test136, _test137, _test138,
	_test139, _test140, _test141, _test142, _test143, _test144, _test145,
	_test146, _test147, _test148, _test149, _test150, _test151, _test152,
	_test153, _test154, _test155, _test156, _test157, _test158, _test159,
	_test160, _test161, _test162, _test163, _test164, _test165, _test166,
	_test167, _test168, _test169, _test170, _test171, _test172, _test173,
	_test174, _test175, _test176, _test177, _test178, _test179, _test180,
	_test181, _test182, _test183, _test184, _test185, _test186, _test187,
	_test188, _test189, _test190, _test191, _test192, _test193, _test194,
	_test195, _test196, _test197, _test198, _test199, _test200, _test201,
	_test202, _test203, _test204, _test205, _test206, _test207, _test208,
	_test209, _test210, _test211, _test212, _test213, _test214, _test215,
	_test216, _test217, _test218, _test219, _test220, _test221, _test222,
	_test223, _test224, _test225, _test226, _test227, _test228, _test229,
	_test230, _test231, _test232, _test233, _test234, _test235, _test236,
	_test237, _test238, _test239, _test240, _test241, _test242, _test243,
	_test244, _test245, _test246, _test247, _test248, _test249, _test250,
	_test251, _test252, _test253, _test254, _test255, _test256, _test257,
	_test258, _test259, _test260, _test261, _test262, _test263, _test264,
	_test265, _test266, _test267, _test268, _test269, _test270, _test271,
	_test272, _test273, _test274, _test275, _test276, _test277, _test278,
	_test279, _test280, _test281, _test282, _test283, _test284, _test285,
	_test286, _test287, _test288, _test289, _test290, _test291, _test292,
	_test293, _test294, _test295, _test296, _test297, _test298, _test299,
	_test300, _test301, _test302, _test303, _test304, _test305, _test306,
	_test307, _test308, _test309, _test310, _test311, _test312, _test313,
	_test314, _test315, _test316, _test317, _test318, _test319, _test320,
	_test321, _test322, _test323, _test324, _test325, _test326, _test327,
	_test328, _test329, _test330, _test331, _test332, _test333, _test334,
	_test335, _test336, _test337, _test338, _test339, _test340, _test341,
	_test342, _test343, _test344, _test345, _test346, _test347, _test348,
	_test349, _test350, _test351, _test352, _test353, _test354, _test355,
	_test356, _test357, _test358, _test359, _test360, _test361, _test362,
	_test363, _test364, _test365, _test366, _test367, _test368, _test369,
	_test370, _test371, _test372, _test373, _test374, _test375, _test376,
	_test377, _test378, _test379, _test380, _test381, _test382, _test383,
	_test384, _test385, _test386, _test387, _test388, _test389, _test390,
	_test391, _test392, _test393, _test394, _test395, _test396, _test397,
	_test398, _test399, _test400, _test401, _test402, _test403, _test404,
	_test405, _test406, _test407, _test408, _test409, _test410, _test411,
	_test412, _test413, _test414, _test415, _test416, _test417, _test418,
	_test419, _test420, _test421, _test422, _test423, _test424, _test425,
	_test426, _test427, _test428, _test429, _test430, _test431, _test432,
	_test433, _test434, _test435, _test436, _test437, _test438, _test439,
	_test440, _test441, _test442, _test443, _test444, _test445, _test446,
	_test447, _test448, _test449, _test450, _test451, _test452, _test453,
	_test454, _test455, _test456, _test457, _test458, _test459, _test460,
	_test461, _test462, _test463, _test464, _test465, _test466, _test467,
	_test468, _test469, _test470, _test471, _test472, _test473, _test474,
	_test475, _test476, _test477, _test478, _test479, _test480, _test481,
	_test482, _test483, _test484, _test485, _test486, _test487, _test488,
	_test489, _test490, _test491, _test492, _test493, _test494, _test495,
	_test496, _test497, _test498, _test499, _test500, _test501, _test502,
	_test503, _test504, _test505, _test506, _test507, _test508, _test509,
	_test510, _test511, _test512, _test513, _test514, _test515, _test516,
	_test517, _test518, _test519, _test520, _test521, _test522, _test523,
	_test524, _test525, _test526, _test527, _test528, _test529, _test530,
	_test531, _test532, _test533, _test534, _test535, _test536, _test537,
	_test538, _test539, _test540, _test541, _test542, _test543, _test544,
	_test545, _test546, _test547, _test548, _test549, _test550, _test551,
	_test552, _test553, _test554, _test555, _test556, _test557, _test558,
	_test559, _test560, _test561, _test562, _test563, _test564, _test565,
	_test566, _test567, _test568, _test569, _test570, _test571, _test572,
	_test573, _test574, _test575, _test576, _test577, _test578, _test579,
	_test580, _test581, _test582, _test583, _test584, _test585, _test586,
	_test587, _test588, _test589, _test590, _test591, _test592, _test593,
	_test594, _test595, _test596, _test597, _test598, _test599, _test600,
	_test601, _test602, _test603, _test604, _test605, _test606, _test607,
	_test608, _test609, _test610, _test611, _test612, _test613, _test614,
	_test615, _test616, _test617, _test618, _test619, _test620, _test621,
	_test622, _test623, _test624, _test625, _test626, _test627, _test628,
	_test629, _test630, _test631, _test632, _test633, _test634, _test635,
	_test636, _test637, _test638, _test639, _test640, _test641, _test642,
	_test643, _test644, _test645, _test646, _test647, _test648, _test649,
	_test650, _test651, _test652, _test653, _test654, _test655, _test656,
	_test657, _test658, _test659, _test660, _test661, _test662, _test663,
	_test664, _test665, _test666, _test667, _test668, _test669, _test670,
	_test671, _test672, _test673, _test674, _test675, _test676, _test677,
	_test678, _test679, _test680, _test681, _test682, _test683, _test684,
	_test685, _test686, _test687, _test688, _test689, _test690, _test691,
	_test692, _test693, _test694, _test695, _test696, _test697, _test698,
	_test699, _test700, _test701, _test702, _test703, _test704, _test705,
	_test706, _test707, _test708, _test709, _test710, _test711, _test712,
	_test713, _test714, _test715, _test716, _test717, _test718, _test719,
	_test720, _test721, _test722, _test723, _test724, _test725, _test726,
	_test727, _test728, _test729, _test730, _test731, _test732, _test733,
	_test734, _test735, _test736, _test737, _test738, _test739, _test740,
	_test741, _test742, _test743, _test744, _test745, _test746, _test747,
	_test748, _test749, _test750, _test751, _test752, _test753, _test754,
	_test755, _test756, _test757, _test758, _test759, _test760, _test761,
	_test762, _test763, _test764, _test765, _test766, _test767, _test768,
	_test769, _test770, _test771, _test772, _test773, _test774, _test775,
	_test776, _test777, _test778, _test779, _test780, _test781, _test782,
	_test783, _test784, _test785, _test786, _test787, _test788, _test789,
	_test790, _test791, _test792, _test793, _test794, _test795, _test796,
	_test797, _test798, _test799, _test800, _test801, _test802, _test803,
	_test804, _test805, _test806, _test807, _test808, _test809, _test810,
	_test811, _test812, _test813, _test814, _test815, _test816, _test817,
	_test818, _test819, _test820, _test821, _test822, _test823, _test824,
	_test825, _test826, _test827, _test828, _test829, _test830, _test831,
	_test832, _test833, _test834, _test835, _test836, _test837, _test838,
	_test839, _test840, _test841, _test842, _test843, _test844, _test845,
	_test846, _test847, _test848, _test849, _test850, _test851, _test852,
	_test853, _test854, _test855, _test856, _test857, _test858, _test859,
	_test860, _test861, _test862, _test863, _test864, _test865, _test866,
	_test867, _test868, _test869, _test870, _test871, _test872, _test873,
	_test874, _test875, _test876, _test877, _test878, _test879, _test880,
	_test881, _test882, _test883, _test884, _test885, _test886, _test887,
	_test888, _test889, _test890, _test891, _test892, _test893, _test894,
	_test895, _test896, _test897, _test898, _test899, _test900, _test901,
	_test902, _test903, _test904, _test905, _test906, _test907, _test908,
	_test909, _test910, _test911, _test912, _test913, _test914, _test915,
	_test916, _test917, _test918, _test919, _test920, _test921, _test922,
	_test923, _test924, _test925, _test926, _test927, _test928, _test929,
	_test930, _test931, _test932, _test933, _test934, _test935, _test936,
	_test937, _test938, _test939, _test940, _test941, _test942, _test943,
	_test944, _test945, _test946, _test947, _test948, _test949, _test950,
	_test951, _test952, _test953, _test954, _test955, _test956, _test957,
	_test958, _test959, _test960, _test961, _test962, _test963, _test964,
	_test965, _test966, _test967, _test968, _test969, _test970, _test971,
	_test972, _test973, _test974, _test975, _test976, _test977, _test978,
	_test979, _test980, _test981, _test982, _test983, _test984, _test985,
	_test986, _test987, _test988, _test989, _test990, _test991, _test992,
	_test993, _test994, _test995, _test996, _test997, _test998, _test999,
	_test1000, _test1001, _test1002, _test1003, _test1004, _test1005,
	_test1006, _test1007, _test1008, _test1009, _test1010, _test1011,
	_test1012, _test1013, _test1014, _test1015, _test1016, _test1017,
	_test1018, _test1019, _test1020, _test1021, _test1022, _test1023;

static void _tests_run_tests(_test_state_t *state) {
	_test_tryrun(_test0);
	_test_tryrun(_test1);
	_test_tryrun(_test2);
	_test_tryrun(_test3);
	_test_tryrun(_test4);
	_test_tryrun(_test5);
	_test_tryrun(_test6);
	_test_tryrun(_test7);
	_test_tryrun(_test8);
	_test_tryrun(_test9);
	_test_tryrun(_test10);
	_test_tryrun(_test11);
	_test_tryrun(_test12);
	_test_tryrun(_test13);
	_test_tryrun(_test14);
	_test_tryrun(_test15);
	_test_tryrun(_test16);
	_test_tryrun(_test17);
	_test_tryrun(_test18);
	_test_tryrun(_test19);
	_test_tryrun(_test20);
	_test_tryrun(_test21);
	_test_tryrun(_test22);
	_test_tryrun(_test23);
	_test_tryrun(_test24);
	_test_tryrun(_test25);
	_test_tryrun(_test26);
	_test_tryrun(_test27);
	_test_tryrun(_test28);
	_test_tryrun(_test29);
	_test_tryrun(_test30);
	_test_tryrun(_test31);
	_test_tryrun(_test32);
	_test_tryrun(_test33);
	_test_tryrun(_test34);
	_test_tryrun(_test35);
	_test_tryrun(_test36);
	_test_tryrun(_test37);
	_test_tryrun(_test38);
	_test_tryrun(_test39);
	_test_tryrun(_test40);
	_test_tryrun(_test41);
	_test_tryrun(_test42);
	_test_tryrun(_test43);
	_test_tryrun(_test44);
	_test_tryrun(_test45);
	_test_tryrun(_test46);
	_test_tryrun(_test47);
	_test_tryrun(_test48);
	_test_tryrun(_test49);
	_test_tryrun(_test50);
	_test_tryrun(_test51);
	_test_tryrun(_test52);
	_test_tryrun(_test53);
	_test_tryrun(_test54);
	_test_tryrun(_test55);
	_test_tryrun(_test56);
	_test_tryrun(_test57);
	_test_tryrun(_test58);
	_test_tryrun(_test59);
	_test_tryrun(_test60);
	_test_tryrun(_test61);
	_test_tryrun(_test62);
	_test_tryrun(_test63);
	_test_tryrun(_test64);
	_test_tryrun(_test65);
	_test_tryrun(_test66);
	_test_tryrun(_test67);
	_test_tryrun(_test68);
	_test_tryrun(_test69);
	_test_tryrun(_test70);
	_test_tryrun(_test71);
	_test_tryrun(_test72);
	_test_tryrun(_test73);
	_test_tryrun(_test74);
	_test_tryrun(_test75);
	_test_tryrun(_test76);
	_test_tryrun(_test77);
	_test_tryrun(_test78);
	_test_tryrun(_test79);
	_test_tryrun(_test80);
	_test_tryrun(_test81);
	_test_tryrun(_test82);
	_test_tryrun(_test83);
	_test_tryrun(_test84);
	_test_tryrun(_test85);
	_test_tryrun(_test86);
	_test_tryrun(_test87);
	_test_tryrun(_test88);
	_test_tryrun(_test89);
	_test_tryrun(_test90);
	_test_tryrun(_test91);
	_test_tryrun(_test92);
	_test_tryrun(_test93);
	_test_tryrun(_test94);
	_test_tryrun(_test95);
	_test_tryrun(_test96);
	_test_tryrun(_test97);
	_test_tryrun(_test98);
	_test_tryrun(_test99);
	_test_tryrun(_test100);
	_test_tryrun(_test101);
	_test_tryrun(_test102);
	_test_tryrun(_test103);
	_test_tryrun(_test104);
	_test_tryrun(_test105);
	_test_tryrun(_test106);
	_test_tryrun(_test107);
	_test_tryrun(_test108);
	_test_tryrun(_test109);
	_test_tryrun(_test110);
	_test_tryrun(_test111);
	_test_tryrun(_test112);
	_test_tryrun(_test113);
	_test_tryrun(_test114);
	_test_tryrun(_test115);
	_test_tryrun(_test116);
	_test_tryrun(_test117);
	_test_tryrun(_test118);
	_test_tryrun(_test119);
	_test_tryrun(_test120);
	_test_tryrun(_test121);
	_test_tryrun(_test122);
	_test_tryrun(_test123);
	_test_tryrun(_test124);
	_test_tryrun(_test125);
	_test_tryrun(_test126);
	_test_tryrun(_test127);
	_test_tryrun(_test128);
	_test_tryrun(_test129);
	_test_tryrun(_test130);
	_test_tryrun(_test131);
	_test_tryrun(_test132);
	_test_tryrun(_test133);
	_test_tryrun(_test134);
	_test_tryrun(_test135);
	_test_tryrun(_test136);
	_test_tryrun(_test137);
	_test_tryrun(_test138);
	_test_tryrun(_test139);
	_test_tryrun(_test140);
	_test_tryrun(_test141);
	_test_tryrun(_test142);
	_test_tryrun(_test143);
	_test_tryrun(_test144);
	_test_tryrun(_test145);
	_test_tryrun(_test146);
	_test_tryrun(_test147);
	_test_tryrun(_test148);
	_test_tryrun(_test149);
	_test_tryrun(_test150);
	_test_tryrun(_test151);
	_test_tryrun(_test152);
	_test_tryrun(_test153);
	_test_tryrun(_test154);
	_test_tryrun(_test155);
	_test_tryrun(_test156);
	_test_tryrun(_test157);
	_test_tryrun(_test158);
	_test_tryrun(_test159);
	_test_tryrun(_test160);
	_test_tryrun(_test161);
	_test_tryrun(_test162);
	_test_tryrun(_test163);
	_test_tryrun(_test164);
	_test_tryrun(_test165);
	_test_tryrun(_test166);
	_test_tryrun(_test167);
	_test_tryrun(_test168);
	_test_tryrun(_test169);
	_test_tryrun(_test170);
	_test_tryrun(_test171);
	_test_tryrun(_test172);
	_test_tryrun(_test173);
	_test_tryrun(_test174);
	_test_tryrun(_test175);
	_test_tryrun(_test176);
	_test_tryrun(_test177);
	_test_tryrun(_test178);
	_test_tryrun(_test179);
	_test_tryrun(_test180);
	_test_tryrun(_test181);
	_test_tryrun(_test182);
	_test_tryrun(_test183);
	_test_tryrun(_test184);
	_test_tryrun(_test185);
	_test_tryrun(_test186);
	_test_tryrun(_test187);
	_test_tryrun(_test188);
	_test_tryrun(_test189);
	_test_tryrun(_test190);
	_test_tryrun(_test191);
	_test_tryrun(_test192);
	_test_tryrun(_test193);
	_test_tryrun(_test194);
	_test_tryrun(_test195);
	_test_tryrun(_test196);
	_test_tryrun(_test197);
	_test_tryrun(_test198);
	_test_tryrun(_test199);
	_test_tryrun(_test200);
	_test_tryrun(_test201);
	_test_tryrun(_test202);
	_test_tryrun(_test203);
	_test_tryrun(_test204);
	_test_tryrun(_test205);
	_test_tryrun(_test206);
	_test_tryrun(_test207);
	_test_tryrun(_test208);
	_test_tryrun(_test209);
	_test_tryrun(_test210);
	_test_tryrun(_test211);
	_test_tryrun(_test212);
	_test_tryrun(_test213);
	_test_tryrun(_test214);
	_test_tryrun(_test215);
	_test_tryrun(_test216);
	_test_tryrun(_test217);
	_test_tryrun(_test218);
	_test_tryrun(_test219);
	_test_tryrun(_test220);
	_test_tryrun(_test221);
	_test_tryrun(_test222);
	_test_tryrun(_test223);
	_test_tryrun(_test224);
	_test_tryrun(_test225);
	_test_tryrun(_test226);
	_test_tryrun(_test227);
	_test_tryrun(_test228);
	_test_tryrun(_test229);
	_test_tryrun(_test230);
	_test_tryrun(_test231);
	_test_tryrun(_test232);
	_test_tryrun(_test233);
	_test_tryrun(_test234);
	_test_tryrun(_test235);
	_test_tryrun(_test236);
	_test_tryrun(_test237);
	_test_tryrun(_test238);
	_test_tryrun(_test239);
	_test_tryrun(_test240);
	_test_tryrun(_test241);
	_test_tryrun(_test242);
	_test_tryrun(_test243);
	_test_tryrun(_test244);
	_test_tryrun(_test245);
	_test_tryrun(_test246);
	_test_tryrun(_test247);
	_test_tryrun(_test248);
	_test_tryrun(_test249);
	_test_tryrun(_test250);
	_test_tryrun(_test251);
	_test_tryrun(_test252);
	_test_tryrun(_test253);
	_test_tryrun(_test254);
	_test_tryrun(_test255);
	_test_tryrun(_test256);
	_test_tryrun(_test257);
	_test_tryrun(_test258);
	_test_tryrun(_test259);
	_test_tryrun(_test260);
	_test_tryrun(_test261);
	_test_tryrun(_test262);
	_test_tryrun(_test263);
	_test_tryrun(_test264);
	_test_tryrun(_test265);
	_test_tryrun(_test266);
	_test_tryrun(_test267);
	_test_tryrun(_test268);
	_test_tryrun(_test269);
	_test_tryrun(_test270);
	_test_tryrun(_test271);
	_test_tryrun(_test272);
	_test_tryrun(_test273);
	_test_tryrun(_test274);
	_test_tryrun(_test275);
	_test_tryrun(_test276);
	_test_tryrun(_test277);
	_test_tryrun(_test278);
	_test_tryrun(_test279);
	_test_tryrun(_test280);
	_test_tryrun(_test281);
	_test_tryrun(_test282);
	_test_tryrun(_test283);
	_test_tryrun(_test284);
	_test_tryrun(_test285);
	_test_tryrun(_test286);
	_test_tryrun(_test287);
	_test_tryrun(_test288);
	_test_tryrun(_test289);
	_test_tryrun(_test290);
	_test_tryrun(_test291);
	_test_tryrun(_test292);
	_test_tryrun(_test293);
	_test_tryrun(_test294);
	_test_tryrun(_test295);
	_test_tryrun(_test296);
	_test_tryrun(_test297);
	_test_tryrun(_test298);
	_test_tryrun(_test299);
	_test_tryrun(_test300);
	_test_tryrun(_test301);
	_test_tryrun(_test302);
	_test_tryrun(_test303);
	_test_tryrun(_test304);
	_test_tryrun(_test305);
	_test_tryrun(_test306);
	_test_tryrun(_test307);
	_test_tryrun(_test308);
	_test_tryrun(_test309);
	_test_tryrun(_test310);
	_test_tryrun(_test311);
	_test_tryrun(_test312);
	_test_tryrun(_test313);
	_test_tryrun(_test314);
	_test_tryrun(_test315);
	_test_tryrun(_test316);
	_test_tryrun(_test317);
	_test_tryrun(_test318);
	_test_tryrun(_test319);
	_test_tryrun(_test320);
	_test_tryrun(_test321);
	_test_tryrun(_test322);
	_test_tryrun(_test323);
	_test_tryrun(_test324);
	_test_tryrun(_test325);
	_test_tryrun(_test326);
	_test_tryrun(_test327);
	_test_tryrun(_test328);
	_test_tryrun(_test329);
	_test_tryrun(_test330);
	_test_tryrun(_test331);
	_test_tryrun(_test332);
	_test_tryrun(_test333);
	_test_tryrun(_test334);
	_test_tryrun(_test335);
	_test_tryrun(_test336);
	_test_tryrun(_test337);
	_test_tryrun(_test338);
	_test_tryrun(_test339);
	_test_tryrun(_test340);
	_test_tryrun(_test341);
	_test_tryrun(_test342);
	_test_tryrun(_test343);
	_test_tryrun(_test344);
	_test_tryrun(_test345);
	_test_tryrun(_test346);
	_test_tryrun(_test347);
	_test_tryrun(_test348);
	_test_tryrun(_test349);
	_test_tryrun(_test350);
	_test_tryrun(_test351);
	_test_tryrun(_test352);
	_test_tryrun(_test353);
	_test_tryrun(_test354);
	_test_tryrun(_test355);
	_test_tryrun(_test356);
	_test_tryrun(_test357);
	_test_tryrun(_test358);
	_test_tryrun(_test359);
	_test_tryrun(_test360);
	_test_tryrun(_test361);
	_test_tryrun(_test362);
	_test_tryrun(_test363);
	_test_tryrun(_test364);
	_test_tryrun(_test365);
	_test_tryrun(_test366);
	_test_tryrun(_test367);
	_test_tryrun(_test368);
	_test_tryrun(_test369);
	_test_tryrun(_test370);
	_test_tryrun(_test371);
	_test_tryrun(_test372);
	_test_tryrun(_test373);
	_test_tryrun(_test374);
	_test_tryrun(_test375);
	_test_tryrun(_test376);
	_test_tryrun(_test377);
	_test_tryrun(_test378);
	_test_tryrun(_test379);
	_test_tryrun(_test380);
	_test_tryrun(_test381);
	_test_tryrun(_test382);
	_test_tryrun(_test383);
	_test_tryrun(_test384);
	_test_tryrun(_test385);
	_test_tryrun(_test386);
	_test_tryrun(_test387);
	_test_tryrun(_test388);
	_test_tryrun(_test389);
	_test_tryrun(_test390);
	_test_tryrun(_test391);
	_test_tryrun(_test392);
	_test_tryrun(_test393);
	_test_tryrun(_test394);
	_test_tryrun(_test395);
	_test_tryrun(_test396);
	_test_tryrun(_test397);
	_test_tryrun(_test398);
	_test_tryrun(_test399);
	_test_tryrun(_test400);
	_test_tryrun(_test401);
	_test_tryrun(_test402);
	_test_tryrun(_test403);
	_test_tryrun(_test404);
	_test_tryrun(_test405);
	_test_tryrun(_test406);
	_test_tryrun(_test407);
	_test_tryrun(_test408);
	_test_tryrun(_test409);
	_test_tryrun(_test410);
	_test_tryrun(_test411);
	_test_tryrun(_test412);
	_test_tryrun(_test413);
	_test_tryrun(_test414);
	_test_tryrun(_test415);
	_test_tryrun(_test416);
	_test_tryrun(_test417);
	_test_tryrun(_test418);
	_test_tryrun(_test419);
	_test_tryrun(_test420);
	_test_tryrun(_test421);
	_test_tryrun(_test422);
	_test_tryrun(_test423);
	_test_tryrun(_test424);
	_test_tryrun(_test425);
	_test_tryrun(_test426);
	_test_tryrun(_test427);
	_test_tryrun(_test428);
	_test_tryrun(_test429);
	_test_tryrun(_test430);
	_test_tryrun(_test431);
	_test_tryrun(_test432);
	_test_tryrun(_test433);
	_test_tryrun(_test434);
	_test_tryrun(_test435);
	_test_tryrun(_test436);
	_test_tryrun(_test437);
	_test_tryrun(_test438);
	_test_tryrun(_test439);
	_test_tryrun(_test440);
	_test_tryrun(_test441);
	_test_tryrun(_test442);
	_test_tryrun(_test443);
	_test_tryrun(_test444);
	_test_tryrun(_test445);
	_test_tryrun(_test446);
	_test_tryrun(_test447);
	_test_tryrun(_test448);
	_test_tryrun(_test449);
	_test_tryrun(_test450);
	_test_tryrun(_test451);
	_test_tryrun(_test452);
	_test_tryrun(_test453);
	_test_tryrun(_test454);
	_test_tryrun(_test455);
	_test_tryrun(_test456);
	_test_tryrun(_test457);
	_test_tryrun(_test458);
	_test_tryrun(_test459);
	_test_tryrun(_test460);
	_test_tryrun(_test461);
	_test_tryrun(_test462);
	_test_tryrun(_test463);
	_test_tryrun(_test464);
	_test_tryrun(_test465);
	_test_tryrun(_test466);
	_test_tryrun(_test467);
	_test_tryrun(_test468);
	_test_tryrun(_test469);
	_test_tryrun(_test470);
	_test_tryrun(_test471);
	_test_tryrun(_test472);
	_test_tryrun(_test473);
	_test_tryrun(_test474);
	_test_tryrun(_test475);
	_test_tryrun(_test476);
	_test_tryrun(_test477);
	_test_tryrun(_test478);
	_test_tryrun(_test479);
	_test_tryrun(_test480);
	_test_tryrun(_test481);
	_test_tryrun(_test482);
	_test_tryrun(_test483);
	_test_tryrun(_test484);
	_test_tryrun(_test485);
	_test_tryrun(_test486);
	_test_tryrun(_test487);
	_test_tryrun(_test488);
	_test_tryrun(_test489);
	_test_tryrun(_test490);
	_test_tryrun(_test491);
	_test_tryrun(_test492);
	_test_tryrun(_test493);
	_test_tryrun(_test494);
	_test_tryrun(_test495);
	_test_tryrun(_test496);
	_test_tryrun(_test497);
	_test_tryrun(_test498);
	_test_tryrun(_test499);
	_test_tryrun(_test500);
	_test_tryrun(_test501);
	_test_tryrun(_test502);
	_test_tryrun(_test503);
	_test_tryrun(_test504);
	_test_tryrun(_test505);
	_test_tryrun(_test506);
	_test_tryrun(_test507);
	_test_tryrun(_test508);
	_test_tryrun(_test509);
	_test_tryrun(_test510);
	_test_tryrun(_test511);
	_test_tryrun(_test512);
	_test_tryrun(_test513);
	_test_tryrun(_test514);
	_test_tryrun(_test515);
	_test_tryrun(_test516);
	_test_tryrun(_test517);
	_test_tryrun(_test518);
	_test_tryrun(_test519);
	_test_tryrun(_test520);
	_test_tryrun(_test521);
	_test_tryrun(_test522);
	_test_tryrun(_test523);
	_test_tryrun(_test524);
	_test_tryrun(_test525);
	_test_tryrun(_test526);
	_test_tryrun(_test527);
	_test_tryrun(_test528);
	_test_tryrun(_test529);
	_test_tryrun(_test530);
	_test_tryrun(_test531);
	_test_tryrun(_test532);
	_test_tryrun(_test533);
	_test_tryrun(_test534);
	_test_tryrun(_test535);
	_test_tryrun(_test536);
	_test_tryrun(_test537);
	_test_tryrun(_test538);
	_test_tryrun(_test539);
	_test_tryrun(_test540);
	_test_tryrun(_test541);
	_test_tryrun(_test542);
	_test_tryrun(_test543);
	_test_tryrun(_test544);
	_test_tryrun(_test545);
	_test_tryrun(_test546);
	_test_tryrun(_test547);
	_test_tryrun(_test548);
	_test_tryrun(_test549);
	_test_tryrun(_test550);
	_test_tryrun(_test551);
	_test_tryrun(_test552);
	_test_tryrun(_test553);
	_test_tryrun(_test554);
	_test_tryrun(_test555);
	_test_tryrun(_test556);
	_test_tryrun(_test557);
	_test_tryrun(_test558);
	_test_tryrun(_test559);
	_test_tryrun(_test560);
	_test_tryrun(_test561);
	_test_tryrun(_test562);
	_test_tryrun(_test563);
	_test_tryrun(_test564);
	_test_tryrun(_test565);
	_test_tryrun(_test566);
	_test_tryrun(_test567);
	_test_tryrun(_test568);
	_test_tryrun(_test569);
	_test_tryrun(_test570);
	_test_tryrun(_test571);
	_test_tryrun(_test572);
	_test_tryrun(_test573);
	_test_tryrun(_test574);
	_test_tryrun(_test575);
	_test_tryrun(_test576);
	_test_tryrun(_test577);
	_test_tryrun(_test578);
	_test_tryrun(_test579);
	_test_tryrun(_test580);
	_test_tryrun(_test581);
	_test_tryrun(_test582);
	_test_tryrun(_test583);
	_test_tryrun(_test584);
	_test_tryrun(_test585);
	_test_tryrun(_test586);
	_test_tryrun(_test587);
	_test_tryrun(_test588);
	_test_tryrun(_test589);
	_test_tryrun(_test590);
	_test_tryrun(_test591);
	_test_tryrun(_test592);
	_test_tryrun(_test593);
	_test_tryrun(_test594);
	_test_tryrun(_test595);
	_test_tryrun(_test596);
	_test_tryrun(_test597);
	_test_tryrun(_test598);
	_test_tryrun(_test599);
	_test_tryrun(_test600);
	_test_tryrun(_test601);
	_test_tryrun(_test602);
	_test_tryrun(_test603);
	_test_tryrun(_test604);
	_test_tryrun(_test605);
	_test_tryrun(_test606);
	_test_tryrun(_test607);
	_test_tryrun(_test608);
	_test_tryrun(_test609);
	_test_tryrun(_test610);
	_test_tryrun(_test611);
	_test_tryrun(_test612);
	_test_tryrun(_test613);
	_test_tryrun(_test614);
	_test_tryrun(_test615);
	_test_tryrun(_test616);
	_test_tryrun(_test617);
	_test_tryrun(_test618);
	_test_tryrun(_test619);
	_test_tryrun(_test620);
	_test_tryrun(_test621);
	_test_tryrun(_test622);
	_test_tryrun(_test623);
	_test_tryrun(_test624);
	_test_tryrun(_test625);
	_test_tryrun(_test626);
	_test_tryrun(_test627);
	_test_tryrun(_test628);
	_test_tryrun(_test629);
	_test_tryrun(_test630);
	_test_tryrun(_test631);
	_test_tryrun(_test632);
	_test_tryrun(_test633);
	_test_tryrun(_test634);
	_test_tryrun(_test635);
	_test_tryrun(_test636);
	_test_tryrun(_test637);
	_test_tryrun(_test638);
	_test_tryrun(_test639);
	_test_tryrun(_test640);
	_test_tryrun(_test641);
	_test_tryrun(_test642);
	_test_tryrun(_test643);
	_test_tryrun(_test644);
	_test_tryrun(_test645);
	_test_tryrun(_test646);
	_test_tryrun(_test647);
	_test_tryrun(_test648);
	_test_tryrun(_test649);
	_test_tryrun(_test650);
	_test_tryrun(_test651);
	_test_tryrun(_test652);
	_test_tryrun(_test653);
	_test_tryrun(_test654);
	_test_tryrun(_test655);
	_test_tryrun(_test656);
	_test_tryrun(_test657);
	_test_tryrun(_test658);
	_test_tryrun(_test659);
	_test_tryrun(_test660);
	_test_tryrun(_test661);
	_test_tryrun(_test662);
	_test_tryrun(_test663);
	_test_tryrun(_test664);
	_test_tryrun(_test665);
	_test_tryrun(_test666);
	_test_tryrun(_test667);
	_test_tryrun(_test668);
	_test_tryrun(_test669);
	_test_tryrun(_test670);
	_test_tryrun(_test671);
	_test_tryrun(_test672);
	_test_tryrun(_test673);
	_test_tryrun(_test674);
	_test_tryrun(_test675);
	_test_tryrun(_test676);
	_test_tryrun(_test677);
	_test_tryrun(_test678);
	_test_tryrun(_test679);
	_test_tryrun(_test680);
	_test_tryrun(_test681);
	_test_tryrun(_test682);
	_test_tryrun(_test683);
	_test_tryrun(_test684);
	_test_tryrun(_test685);
	_test_tryrun(_test686);
	_test_tryrun(_test687);
	_test_tryrun(_test688);
	_test_tryrun(_test689);
	_test_tryrun(_test690);
	_test_tryrun(_test691);
	_test_tryrun(_test692);
	_test_tryrun(_test693);
	_test_tryrun(_test694);
	_test_tryrun(_test695);
	_test_tryrun(_test696);
	_test_tryrun(_test697);
	_test_tryrun(_test698);
	_test_tryrun(_test699);
	_test_tryrun(_test700);
	_test_tryrun(_test701);
	_test_tryrun(_test702);
	_test_tryrun(_test703);
	_test_tryrun(_test704);
	_test_tryrun(_test705);
	_test_tryrun(_test706);
	_test_tryrun(_test707);
	_test_tryrun(_test708);
	_test_tryrun(_test709);
	_test_tryrun(_test710);
	_test_tryrun(_test711);
	_test_tryrun(_test712);
	_test_tryrun(_test713);
	_test_tryrun(_test714);
	_test_tryrun(_test715);
	_test_tryrun(_test716);
	_test_tryrun(_test717);
	_test_tryrun(_test718);
	_test_tryrun(_test719);
	_test_tryrun(_test720);
	_test_tryrun(_test721);
	_test_tryrun(_test722);
	_test_tryrun(_test723);
	_test_tryrun(_test724);
	_test_tryrun(_test725);
	_test_tryrun(_test726);
	_test_tryrun(_test727);
	_test_tryrun(_test728);
	_test_tryrun(_test729);
	_test_tryrun(_test730);
	_test_tryrun(_test731);
	_test_tryrun(_test732);
	_test_tryrun(_test733);
	_test_tryrun(_test734);
	_test_tryrun(_test735);
	_test_tryrun(_test736);
	_test_tryrun(_test737);
	_test_tryrun(_test738);
	_test_tryrun(_test739);
	_test_tryrun(_test740);
	_test_tryrun(_test741);
	_test_tryrun(_test742);
	_test_tryrun(_test743);
	_test_tryrun(_test744);
	_test_tryrun(_test745);
	_test_tryrun(_test746);
	_test_tryrun(_test747);
	_test_tryrun(_test748);
	_test_tryrun(_test749);
	_test_tryrun(_test750);
	_test_tryrun(_test751);
	_test_tryrun(_test752);
	_test_tryrun(_test753);
	_test_tryrun(_test754);
	_test_tryrun(_test755);
	_test_tryrun(_test756);
	_test_tryrun(_test757);
	_test_tryrun(_test758);
	_test_tryrun(_test759);
	_test_tryrun(_test760);
	_test_tryrun(_test761);
	_test_tryrun(_test762);
	_test_tryrun(_test763);
	_test_tryrun(_test764);
	_test_tryrun(_test765);
	_test_tryrun(_test766);
	_test_tryrun(_test767);
	_test_tryrun(_test768);
	_test_tryrun(_test769);
	_test_tryrun(_test770);
	_test_tryrun(_test771);
	_test_tryrun(_test772);
	_test_tryrun(_test773);
	_test_tryrun(_test774);
	_test_tryrun(_test775);
	_test_tryrun(_test776);
	_test_tryrun(_test777);
	_test_tryrun(_test778);
	_test_tryrun(_test779);
	_test_tryrun(_test780);
	_test_tryrun(_test781);
	_test_tryrun(_test782);
	_test_tryrun(_test783);
	_test_tryrun(_test784);
	_test_tryrun(_test785);
	_test_tryrun(_test786);
	_test_tryrun(_test787);
	_test_tryrun(_test788);
	_test_tryrun(_test789);
	_test_tryrun(_test790);
	_test_tryrun(_test791);
	_test_tryrun(_test792);
	_test_tryrun(_test793);
	_test_tryrun(_test794);
	_test_tryrun(_test795);
	_test_tryrun(_test796);
	_test_tryrun(_test797);
	_test_tryrun(_test798);
	_test_tryrun(_test799);
	_test_tryrun(_test800);
	_test_tryrun(_test801);
	_test_tryrun(_test802);
	_test_tryrun(_test803);
	_test_tryrun(_test804);
	_test_tryrun(_test805);
	_test_tryrun(_test806);
	_test_tryrun(_test807);
	_test_tryrun(_test808);
	_test_tryrun(_test809);
	_test_tryrun(_test810);
	_test_tryrun(_test811);
	_test_tryrun(_test812);
	_test_tryrun(_test813);
	_test_tryrun(_test814);
	_test_tryrun(_test815);
	_test_tryrun(_test816);
	_test_tryrun(_test817);
	_test_tryrun(_test818);
	_test_tryrun(_test819);
	_test_tryrun(_test820);
	_test_tryrun(_test821);
	_test_tryrun(_test822);
	_test_tryrun(_test823);
	_test_tryrun(_test824);
	_test_tryrun(_test825);
	_test_tryrun(_test826);
	_test_tryrun(_test827);
	_test_tryrun(_test828);
	_test_tryrun(_test829);
	_test_tryrun(_test830);
	_test_tryrun(_test831);
	_test_tryrun(_test832);
	_test_tryrun(_test833);
	_test_tryrun(_test834);
	_test_tryrun(_test835);
	_test_tryrun(_test836);
	_test_tryrun(_test837);
	_test_tryrun(_test838);
	_test_tryrun(_test839);
	_test_tryrun(_test840);
	_test_tryrun(_test841);
	_test_tryrun(_test842);
	_test_tryrun(_test843);
	_test_tryrun(_test844);
	_test_tryrun(_test845);
	_test_tryrun(_test846);
	_test_tryrun(_test847);
	_test_tryrun(_test848);
	_test_tryrun(_test849);
	_test_tryrun(_test850);
	_test_tryrun(_test851);
	_test_tryrun(_test852);
	_test_tryrun(_test853);
	_test_tryrun(_test854);
	_test_tryrun(_test855);
	_test_tryrun(_test856);
	_test_tryrun(_test857);
	_test_tryrun(_test858);
	_test_tryrun(_test859);
	_test_tryrun(_test860);
	_test_tryrun(_test861);
	_test_tryrun(_test862);
	_test_tryrun(_test863);
	_test_tryrun(_test864);
	_test_tryrun(_test865);
	_test_tryrun(_test866);
	_test_tryrun(_test867);
	_test_tryrun(_test868);
	_test_tryrun(_test869);
	_test_tryrun(_test870);
	_test_tryrun(_test871);
	_test_tryrun(_test872);
	_test_tryrun(_test873);
	_test_tryrun(_test874);
	_test_tryrun(_test875);
	_test_tryrun(_test876);
	_test_tryrun(_test877);
	_test_tryrun(_test878);
	_test_tryrun(_test879);
	_test_tryrun(_test880);
	_test_tryrun(_test881);
	_test_tryrun(_test882);
	_test_tryrun(_test883);
	_test_tryrun(_test884);
	_test_tryrun(_test885);
	_test_tryrun(_test886);
	_test_tryrun(_test887);
	_test_tryrun(_test888);
	_test_tryrun(_test889);
	_test_tryrun(_test890);
	_test_tryrun(_test891);
	_test_tryrun(_test892);
	_test_tryrun(_test893);
	_test_tryrun(_test894);
	_test_tryrun(_test895);
	_test_tryrun(_test896);
	_test_tryrun(_test897);
	_test_tryrun(_test898);
	_test_tryrun(_test899);
	_test_tryrun(_test900);
	_test_tryrun(_test901);
	_test_tryrun(_test902);
	_test_tryrun(_test903);
	_test_tryrun(_test904);
	_test_tryrun(_test905);
	_test_tryrun(_test906);
	_test_tryrun(_test907);
	_test_tryrun(_test908);
	_test_tryrun(_test909);
	_test_tryrun(_test910);
	_test_tryrun(_test911);
	_test_tryrun(_test912);
	_test_tryrun(_test913);
	_test_tryrun(_test914);
	_test_tryrun(_test915);
	_test_tryrun(_test916);
	_test_tryrun(_test917);
	_test_tryrun(_test918);
	_test_tryrun(_test919);
	_test_tryrun(_test920);
	_test_tryrun(_test921);
	_test_tryrun(_test922);
	_test_tryrun(_test923);
	_test_tryrun(_test924);
	_test_tryrun(_test925);
	_test_tryrun(_test926);
	_test_tryrun(_test927);
	_test_tryrun(_test928);
	_test_tryrun(_test929);
	_test_tryrun(_test930);
	_test_tryrun(_test931);
	_test_tryrun(_test932);
	_test_tryrun(_test933);
	_test_tryrun(_test934);
	_test_tryrun(_test935);
	_test_tryrun(_test936);
	_test_tryrun(_test937);
	_test_tryrun(_test938);
	_test_tryrun(_test939);
	_test_tryrun(_test940);
	_test_tryrun(_test941);
	_test_tryrun(_test942);
	_test_tryrun(_test943);
	_test_tryrun(_test944);
	_test_tryrun(_test945);
	_test_tryrun(_test946);
	_test_tryrun(_test947);
	_test_tryrun(_test948);
	_test_tryrun(_test949);
	_test_tryrun(_test950);
	_test_tryrun(_test951);
	_test_tryrun(_test952);
	_test_tryrun(_test953);
	_test_tryrun(_test954);
	_test_tryrun(_test955);
	_test_tryrun(_test956);
	_test_tryrun(_test957);
	_test_tryrun(_test958);
	_test_tryrun(_test959);
	_test_tryrun(_test960);
	_test_tryrun(_test961);
	_test_tryrun(_test962);
	_test_tryrun(_test963);
	_test_tryrun(_test964);
	_test_tryrun(_test965);
	_test_tryrun(_test966);
	_test_tryrun(_test967);
	_test_tryrun(_test968);
	_test_tryrun(_test969);
	_test_tryrun(_test970);
	_test_tryrun(_test971);
	_test_tryrun(_test972);
	_test_tryrun(_test973);
	_test_tryrun(_test974);
	_test_tryrun(_test975);
	_test_tryrun(_test976);
	_test_tryrun(_test977);
	_test_tryrun(_test978);
	_test_tryrun(_test979);
	_test_tryrun(_test980);
	_test_tryrun(_test981);
	_test_tryrun(_test982);
	_test_tryrun(_test983);
	_test_tryrun(_test984);
	_test_tryrun(_test985);
	_test_tryrun(_test986);
	_test_tryrun(_test987);
	_test_tryrun(_test988);
	_test_tryrun(_test989);
	_test_tryrun(_test990);
	_test_tryrun(_test991);
	_test_tryrun(_test992);
	_test_tryrun(_test993);
	_test_tryrun(_test994);
	_test_tryrun(_test995);
	_test_tryrun(_test996);
	_test_tryrun(_test997);
	_test_tryrun(_test998);
	_test_tryrun(_test999);
	_test_tryrun(_test1000);
	_test_tryrun(_test1001);
	_test_tryrun(_test1002);
	_test_tryrun(_test1003);
	_test_tryrun(_test1004);
	_test_tryrun(_test1005);
	_test_tryrun(_test1006);
	_test_tryrun(_test1007);
	_test_tryrun(_test1008);
	_test_tryrun(_test1009);
	_test_tryrun(_test1010);
	_test_tryrun(_test1011);
	_test_tryrun(_test1012);
	_test_tryrun(_test1013);
	_test_tryrun(_test1014);
	_test_tryrun(_test1015);
	_test_tryrun(_test1016);
	_test_tryrun(_test1017);
	_test_tryrun(_test1018);
	_test_tryrun(_test1019);
	_test_tryrun(_test1020);
	_test_tryrun(_test1021);
	_test_tryrun(_test1022);
	_test_tryrun(_test1023);
}
