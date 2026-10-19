#include <stdio.h>
#include <zephyr/ztest.h>

struct shim_case shim_cases[512];
int shim_case_count;
int shim_failures;

int main(void)
{
	for (int i = 0; i < shim_case_count; i++) {
		int before = shim_failures;

		shim_cases[i].fn();
		printf("%s %s\n", shim_failures == before ? "PASS" : "FAIL", shim_cases[i].name);
	}
	printf("HOST_TESTS cases=%d failures=%d\n", shim_case_count, shim_failures);
	return shim_failures != 0;
}
