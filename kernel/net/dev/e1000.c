#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "arch/riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "pci.h"
#include "net/dev/e1000_dev.h"
#include "net/mbuf.h"
#include "net/ethernet.h"

// remember where the e1000's registers live.
static volatile uint32_t *regs;

static void e1000_init_core(uint32_t* xregs);

static int e1000_pci_init(struct pci_dev* dev) {
	dev->base[1] = 7;
	__sync_synchronize();

	for(int i = 0; i < 6; i++) {
		uint32_t old = dev->base[4+i];
		dev->base[4+i] = 0xffffffff;
		__sync_synchronize();
		dev->base[4+i] = old;
	}

	dev->base[4+0] = (uint32_t) E1000_REG;

	printf("e1000_init start\n");
	e1000_init_core((uint32_t *)E1000_REG);

	return 0;
}

static struct pci_driver e1000_driver = {
	.name   = "82540EM Gigabit Ethernet Controller driver",
	.init = e1000_pci_init
};
static struct pci_dev e1000_dev = {
	.name  = "82540EM Gigabit Ethernet Controller",
	.id    = ID_82540EM,
	.base  = 0,
	.driver = &e1000_driver
};

void pci_register_e1000() {
	pci_register_device(&e1000_dev);
}

<<<<<<< HEAD
static struct netdev* alloc_netdev(uint32_t* xregs) {
	struct netdev* ndev;
	ndev = ufkalloc(sizeof(*ndev));
	for (int i = 0; i < j)
}

=======
>>>>>>> cd681140f3e5ff92308e724a8148f89a009989d6
static void e1000_init_core(uint32_t* xregs) {
	int i;

	initlock(&e1000_lock, "e1000");

	regs = xregs;

	// Reset the device
	regs[E1000_IMS] = 0; // disable interrupts
	regs[E1000_CTL] |= E1000_CTL_RST;
	regs[E1000_IMS] = 0; // redisable interrupts
	__sync_synchronize();

	// [E1000 14.5] Transmit initialization
	memset(tx_ring, 0, sizeof(tx_ring));
	for (i = 0; i < TX_RING_SIZE; i++) {
		tx_ring[i].status = E1000_TXD_STAT_DD;
		tx_mbuf[i] = 0;
	}
	regs[E1000_TDBAL] = (uint64_t) tx_ring;
	if(sizeof(tx_ring) % 128 != 0)
		panic("e1000");
	regs[E1000_TDLEN] = sizeof(tx_ring);
	regs[E1000_TDH] = regs[E1000_TDT] = 0;
	
	// [E1000 14.4] Receive initialization
	memset(rx_ring, 0, sizeof(rx_ring));
	for (i = 0; i < RX_RING_SIZE; i++) {
		rx_ring[i].addr = (uint64_t) rx_mbuf[i].buf;
	}
	regs[E1000_RDBAL] = (uint64_t) rx_ring;
	if(sizeof(rx_ring) % 128 != 0)
		panic("e1000");
	regs[E1000_RDH] = 0;
	regs[E1000_RDT] = RX_RING_SIZE - 1;
	regs[E1000_RDLEN] = sizeof(rx_ring);

	// filter by qemu's MAC address, 52:54:00:12:34:56
	regs[E1000_RA] = 0x12005452;
	regs[E1000_RA+1] = 0x5634 | (1<<31);
	// multicast table
	for (int i = 0; i < 4096/32; i++)
		regs[E1000_MTA + i] = 0;

	// transmitter control bits.
	regs[E1000_TCTL] = E1000_TCTL_EN |  // enable
		E1000_TCTL_PSP |                  // pad short packets
		(0x10 << E1000_TCTL_CT_SHIFT) |   // collision stuff
		(0x40 << E1000_TCTL_COLD_SHIFT);
	regs[E1000_TIPG] = 10 | (8<<10) | (6<<20); // inter-pkt gap

	// receiver control bits.
	regs[E1000_RCTL] = E1000_RCTL_EN | // enable receiver
		E1000_RCTL_BAM |                 // enable broadcast
		E1000_RCTL_SZ_2048 |             // 2048-byte rx buffers
		E1000_RCTL_SECRC;                // strip CRC
	
	// ask e1000 for receive interrupts.
	regs[E1000_RDTR] = 0; // interrupt after every received packet (no timer)
	regs[E1000_RADV] = 0; // interrupt after every packet (no timer)
	regs[E1000_IMS] = (1 << 7); // RXDW -- Receiver Descriptor Write Back
}

int e1000_transmit(struct mbuf *m) {
	int index = regs[E1000_TDT];
	
	if(!tx_ring[index].status & E1000_TXD_STAT_DD) {
		printf("[e1000_transmit] still in progress\n");
		return -1;
	}
	
	// free mbuf
	struct mbuf *prev_mbuf = tx_mbuf[index == 0 ? TX_RING_SIZE-1: index-1];
	if (prev_mbuf != 0) {
		mbuffree(prev_mbuf);
		tx_mbuf[index == 0 ? TX_RING_SIZE-1: index-1] = 0;
	}

	tx_ring[index].addr = (uint64_t) m->head;
	tx_ring[index].length = (uint16_t) m->len;
	tx_ring[index].cmd = E1000_TXD_CMD_RS | E1000_TXD_CMD_EOP;
	tx_ring[index].status = 0;

	tx_mbuf[index] = m;

	regs[E1000_TDT] = (index + 1) % TX_RING_SIZE;

	return 0;
}

static void e1000_recv(void) {
	// init
	int index = (regs[E1000_RDT]+1) % RX_RING_SIZE;
	if (rx_ring[index].status & E1000_RXD_STAT_DD) {
		struct mbuf *m = &rx_mbuf[index];
		uint16_t len = rx_ring[index].length;
		rx_ring[index].status ^= E1000_RXD_STAT_DD;

		m->raddr = 0;
		m->next = 0;
		m->head = m->buf;
		m->len = 0;
		mbufput(m, len);

		regs[E1000_RDT] = index;
		index = (index+1) % RX_RING_SIZE;
		eth_recv(m);
	}
}

void e1000_intr() {
	regs[E1000_ICR]; // clear pending interrupts
	e1000_recv();
}
