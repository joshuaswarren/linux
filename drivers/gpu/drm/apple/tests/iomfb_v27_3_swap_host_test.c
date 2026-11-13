/* Host test: cc -I. -I<dir holding stub linux/string.h and linux/types.h> tests/iomfb_v27_3_swap_host_test.c */
#include <stdio.h>
#include <stdint.h>
typedef uint8_t u8;
#include "iomfb_v27_3_swap.h"

int main(void)
{
	static u8 src[DCP_V27_REQ_SIZE], dst[DCP_V27_3_REQ_SIZE];
	int bad = 0;

	src[0x98] = 0x11;			/* swap_id */
	src[DCP_V27_SWAP_SIZE] = 0x22;		/* first surface byte */
	src[DCP_V27_REQ_SIZE - 1] = 0x33;	/* last tail byte */
	dcp_v27_3_swap_req(dst, src);
	bad += dst[0xd8] != 0x11 || dst[0x98] != 0;
	bad += dst[DCP_V27_3_SWAP_SIZE] != 0x22;
	bad += dst[DCP_V27_3_REQ_SIZE - 1] != 0x33;
	bad += DCP_V27_3_REQ_SIZE - DCP_V27_REQ_SIZE != DCP_V27_3_SWAP_SIZE - DCP_V27_SWAP_SIZE;
	puts(bad ? "FAILED" : "ok");
	return bad;
}
