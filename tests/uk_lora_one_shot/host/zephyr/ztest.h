/* Minimal ztest shim so the policy tests run on a plain host C compiler (no Zephyr, no radio). */
#ifndef HOST_ZTEST_SHIM_H_
#define HOST_ZTEST_SHIM_H_

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ARG_UNUSED(x) (void)(x)

struct shim_case {
	const char *name;
	void (*fn)(void);
};
extern struct shim_case shim_cases[512];
extern int shim_case_count;
extern int shim_failures;

#define ZTEST(suite, name)                                                                         \
	static void suite##_##name(void);                                                          \
	__attribute__((constructor)) static void reg_##suite##_##name(void)                        \
	{                                                                                          \
		shim_cases[shim_case_count++] = (struct shim_case){ #suite "." #name,              \
								   suite##_##name };               \
	}                                                                                          \
	static void suite##_##name(void)

#define ZTEST_SUITE(...) struct shim_unused_suite_decl

#define SHIM_FAIL(expr)                                                                            \
	do {                                                                                       \
		printf("  ASSERT FAILED %s:%d: %s\n", __FILE__, __LINE__, expr);                    \
		shim_failures++;                                                                   \
		return;                                                                            \
	} while (0)

#define zassert_true(c, ...) do { if (!(c)) SHIM_FAIL(#c); } while (0)
#define zassert_false(c, ...) do { if (c) SHIM_FAIL("!(" #c ")"); } while (0)
#define zassert_equal(a, b, ...) do { if ((long long)(a) != (long long)(b)) SHIM_FAIL(#a " == " #b); } while (0)
#define zassert_not_equal(a, b, ...) do { if ((long long)(a) == (long long)(b)) SHIM_FAIL(#a " != " #b); } while (0)
#define zassert_mem_equal(a, b, n, ...) do { if (memcmp((a), (b), (n)) != 0) SHIM_FAIL("mem " #a); } while (0)

#endif
