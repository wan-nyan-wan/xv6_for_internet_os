#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "arch/riscv.h"
#include "defs.h"
#include "file.h"
#include "net/socket.h"
#include "lib/hashmap.h"

volatile static int started = 0;

// start() jumps here in supervisor mode on all CPUs.
void
main()
{
	if(cpuid() == 0){
		consoleinit();
		printfinit();
		printf("\n");
		printf("xv6 kernel is booting\n");
		printf("\n");
		ufkinit();		// initialize Useful Kernel Allocator
		kvminit();       	// create kernel page table
		kvminithart();   	// turn on paging
		procinit();		// process table
		hashmap_set_allocator(ufkalloc, ufkfree); // initialize hashmap allocator
		trapinit();		// trap vectors
		trapinithart();		// install kernel trap vector
		plicinit();		// set up interrupt controller
		plicinithart();		// ask PLIC for device interrupts
		binit();		// buffer cache
		iinit();		// inode cache
		fileinit();		// file table
		virtio_disk_init(); 	// emulated hard disk
		pci_init();
		pci_register_e1000();
		arpinit();
		tcpinit();
		socket_init();
		userinit();      	// first user process
		__sync_synchronize();
		started = 1;
	} else {
		while(started == 0)
			;
		__sync_synchronize();
		printf("hart %d starting\n", cpuid());
		kvminithart();    // turn on paging
		trapinithart();   // install kernel trap vector
		plicinithart();   // ask PLIC for device interrupts
	}

	scheduler();        
}
