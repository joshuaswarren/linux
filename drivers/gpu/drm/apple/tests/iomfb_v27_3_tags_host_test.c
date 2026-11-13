/* Host test: cc -Wall -o t iomfb_v27_3_tags_host_test.c && ./t */
#include <stdio.h>
#include <string.h>
#include "iomfb_v27_3_tags.h"
static int bad;
static void want(const char *old, const char *new)
{
	const char *got = dcp_v27_3_tag(old);
	if (new ? (!got || strncmp(got, new, 4)) : got != NULL) { printf("FAIL %.4s -> %s (want %s)\n", old, got ? got : "none", new ? new : "none"); bad++; }
}
int main(void)
{
	want("A406", "A456"); want("A408", "A458"); want("A411", "A461");
	want("A487", "A538"); want("A489", "A540"); want("A390", "A391");
	want("A033", NULL); want("A000", NULL); want("A136", NULL);
	puts(bad ? "FAILED" : "ok");
	return bad;
}
