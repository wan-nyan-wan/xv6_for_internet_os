#include "types.h"
#include "arch/riscv.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "sys/syscall.h"

uint64_t
sys_exit(void)
{
	int n;
	if(argint(0, &n) < 0)
		return -1;
	exit(n);
	return 0;  // not reached
}

uint64_t
sys_getpid(void)
{
	return myproc()->pid;
}

uint64_t
sys_fork(void)
{
	return fork();
}

uint64_t
sys_wait(void)
{
	uint64_t p;
	if(argaddr(0, &p) < 0)
		return -1;
	return wait(p);
}

uint64_t
sys_sbrk(void)
{
	int addr;
	int n;

	if(argint(0, &n) < 0)
		return -1;
	addr = myproc()->sz;
	if(growproc(n) < 0)
		return -1;
	return addr;
}

uint64_t
sys_sleep(void)
{
	int n;
	uint32_t ticks0;

	if(argint(0, &n) < 0)
		return -1;
	acquire(&tickslock);
	ticks0 = ticks;
	while(ticks - ticks0 < n){
		if(myproc()->killed){
			release(&tickslock);
			return -1;
		}
		sleep(&ticks, &tickslock);
	}
	release(&tickslock);
	return 0;
}

uint64_t
sys_kill(void)
{
	int pid;

	if(argint(0, &pid) < 0)
		return -1;
	return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64_t
sys_uptime(void)
{
	uint32_t xticks;

	acquire(&tickslock);
	xticks = ticks;
	release(&tickslock);
	return xticks;
}
