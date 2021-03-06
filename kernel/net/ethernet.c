#include "types.h"
#include "param.h"
#include "arch/riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "net/byteorder.h"
#include "net/mbuf.h"
#include "net/dev/netdev.h"
#include "net/ethernet.h"
#include "net/arptable.h"
#include "net/arp.h"
#include "net/ipv4.h"
#include "net/netutil.h"

extern struct mbufq arp_q;
extern struct netdev* e1000ndev;

uint8_t local_mac[ETH_ADDR_LEN] = { 0x52, 0x54, 0x00, 0x12, 0x34, 0x56 };
uint8_t broadcast_mac[ETH_ADDR_LEN] = { 0xFF, 0XFF, 0XFF, 0XFF, 0XFF, 0XFF };

// sends an ethernet packet
void
eth_send(struct mbuf *m, uint16_t ethtype, uint32_t dip)
{
	struct eth *ethhdr;

	uint8_t dhost[ETH_ADDR_LEN] = {0, 0, 0, 0, 0, 0};
	if (arptable_get_mac(dip, (uint8_t *)dhost) == -1) {
		if (ethtype != ETH_TYPE_ARP) {
			arp_send(ARP_OP_REQUEST, broadcast_mac, dip);
			mbufq_pushtail(&arp_q, m);
			return;
		} else {
			memmove(dhost, broadcast_mac, ETH_ADDR_LEN);
		}
	}
	ethhdr = mbufpushhdr(m, *ethhdr);
	ethhdr->type = htons(ethtype);
	memmove(ethhdr->shost, local_mac, ETH_ADDR_LEN);
	memmove(ethhdr->dhost, dhost, ETH_ADDR_LEN);

	if (e1000ndev->transmit(m) == -1) {
		mbuffree(m);
	};
}

// called by e1000 driver's interrupt handler to deliver a packet to the
// networking stack
void eth_recv(struct mbuf *m)
{
	struct eth *ethhdr;
	uint16_t type;

	ethhdr = mbufpullhdr(m, *ethhdr);
	if (!ethhdr) {
		return;
	}

	type = ntohs(ethhdr->type);
	if (type == ETH_TYPE_IP) {
		ip_recv(m);
	} else if (type == ETH_TYPE_ARP) {
		arp_recv(m);
	}
}
