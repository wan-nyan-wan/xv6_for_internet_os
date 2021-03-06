#include "types.h"
#include "param.h"
#include "arch/riscv.h"
#include "defs.h"
#include "spinlock.h"

void test_start();
void buddy_test();
void sysnet_test();
void tcp_test();
void arp_test();
void sock_cb_test();

volatile static int single_done = 0;

void test_start() {
	printf("start testing...\n\n");

	if (cpuid() == 0) {
		// buddy_test();
		arp_test();
		sock_cb_test();
		sysnet_test();
		single_done = 1;
	} else {
		while (!single_done)
			;
	}
	tcp_test();
	printf("test done!!!\n\n");
}
