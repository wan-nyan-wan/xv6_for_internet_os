#include "types.h"
#include "param.h"
#include "arch/riscv.h"
#include "spinlock.h"
#include "defs.h"
#include "net/mbuf.h"
#include "net/ethernet.h"
#include "net/ipv4.h"
#include "net/tcp.h"
#include "net/sock_cb.h"

extern struct mbufq tx_queue;
void nic_mock_recv(struct mbuf *m);
uint64 sockconnect(struct sock_cb *scb, uint32 raddr, uint16 dport);
uint64 socklisten(struct sock_cb *scb, uint16 sport);

struct mbuf* create_packet(char *bytes, int len) {
  struct mbuf *buf = mbufalloc(ETH_MAX_SIZE);
  mbufput(buf, len);
  memmove((void *)buf->head, (void *)bytes, len);
  return buf;
}

void listen_handshake_test();
void connect_handshake_test();

void tcp_test() {
  printf("\t[tcp test] start...\n");
  listen_handshake_test();
  connect_handshake_test();
  printf("\t[tcp test] done...\n");
}

volatile static int sw = 0;
void wait_process() {
  while(sw == cpuid());
}
void switch_process() {
  sw = cpuid();
}

void listen_handshake_test() {
  // TODO NEED FIX
  struct sock_cb *scb = 0;
  printf("\t\t[listen_handshake test] start...\n");
  if (cpuid() == 0) {

    uint16 sport = 2000;
    scb = alloc_sock_cb(0, 0, 0, 0, SOCK_TCP);
    if (socklisten(scb, sport) < 0) {
      panic("sys_socklisten_core failed!");
    }
  } else if (cpuid() == 1) {
    sw = cpuid();
    char syn_packet_bytes[] = {
      0x52, 0x54, 0x00, 0x12, 0x34, 0x56, 0x5e, 0x02,
      0x03, 0x04, 0x05, 0x06, 0x08, 0x00, 0x45, 0x00,
      0x00, 0x3c, 0x92, 0x61, 0x40, 0x00, 0x40, 0x06,
      0x0e, 0x06, 0xc0, 0xa8, 0x03, 0x02, 0xc0, 0xa8,
      0x16, 0x02, 0x9a, 0xc8, 0x07, 0xd0, 0xb5, 0x29,
      0x39, 0xca, 0x00, 0x00, 0x00, 0x00, 0xa0, 0x02,
      0xfa, 0xf0, 0x16, 0xe2, 0x00, 0x00, 0x02, 0x04,
      0x05, 0xb4, 0x04, 0x02, 0x08, 0x0a, 0x07, 0xe3,
      0x02, 0x69, 0x00, 0x00, 0x00, 0x00, 0x01, 0x03,
      0x03, 0x07
    };
    char syn_ack_packet_bytes[] = {
      0x5e, 0x02, 0x03, 0x04, 0x05, 0x06, 0x52, 0x54,
      0x00, 0x12, 0x34, 0x56, 0x08, 0x00, 0x45, 0x00,
      0x00, 0x28, 0x00, 0x00, 0x00, 0x00, 0x64, 0x06,
      0xbc, 0x7b, 0xc0, 0xa8, 0x16, 0x02, 0xc0, 0xa8,
      0x03, 0x02, 0x07, 0xd0, 0x9a, 0xc8, 0x00, 0x00,
      0x00, 0x00, 0xb5, 0x29, 0x39, 0xcb, 0x50, 0x12,
      0x10, 0x00, 0x73, 0xf0, 0x00, 0x00
    };
    char ack_packet_bytes[] = {
      0x52, 0x54, 0x00, 0x12, 0x34, 0x56, 0x5e, 0x02,
      0x03, 0x04, 0x05, 0x06, 0x08, 0x00, 0x45, 0x00,
      0x00, 0x28, 0x92, 0x63, 0x40, 0x00, 0x40, 0x06,
      0x0e, 0x18, 0xc0, 0xa8, 0x03, 0x02, 0xc0, 0xa8,
      0x16, 0x02, 0x9a, 0xc8, 0x07, 0xd0, 0xb5, 0x29,
      0x39, 0xcb, 0x00, 0x00, 0x00, 0x01, 0x50, 0x10,
      0xfa, 0xf0, 0x89, 0x00, 0x00, 0x00
    };

    wait_process();

    // syn packet
    struct mbuf *syn = create_packet(syn_packet_bytes, sizeof(syn_packet_bytes));
    nic_mock_recv(syn);

    // syn ack packet
    struct mbuf *syn_ack = mbufq_pophead(&tx_queue);
    if (syn_ack == 0) {
      panic("syn ack packet did not send");
    }
    if (memcmp(syn_ack->head, syn_ack_packet_bytes, syn_ack->len) != 0) {
      panic("not match syn_ack");
    }
    mbuffree(syn_ack);

    // ack
    struct mbuf *ack = create_packet(ack_packet_bytes, sizeof(ack_packet_bytes));
    nic_mock_recv(ack);

    // no packet transmit
    if (mbufq_pophead(&tx_queue) != 0) {
      panic("why transmit a packet??");
    }

    if (scb->state != SOCK_CB_ESTAB) {
      panic("why scb is not ESTAB?");
    }

    free_sock_cb(scb);
  }
  printf("\t\t[listen_handshake test] done...!\n");
}

void connect_handshake_test() {
  printf("\t\t[connect_handshake test] start...\n");

  uint32 raddr = MAKE_IP_ADDR(192, 168, 22, 3);
  uint16 dport = 2003;
  struct sock_cb *scb = alloc_sock_cb(0, 0, 0, 0, SOCK_TCP);
  if (sockconnect(scb, raddr, dport) < 0) {
    panic("sys_socklisten_core failed!");
  }

  char arp_request_packet_bytes[] = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x52, 0x54,
    0x00, 0x12, 0x34, 0x56, 0x08, 0x06, 0x00, 0x01,
    0x08, 0x00, 0x06, 0x04, 0x00, 0x01, 0x52, 0x54,
    0x00, 0x12, 0x34, 0x56, 0xc0, 0xa8, 0x16, 0x02,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xc0, 0xa8,
    0x16, 0x03
  };
  char arp_reply_packet_bytes[] = {
    0x52, 0x54, 0x00, 0x12, 0x34, 0x56, 0x5e, 0x02,
    0x03, 0x04, 0x05, 0x06, 0x08, 0x06, 0x00, 0x01,
    0x08, 0x00, 0x06, 0x04, 0x00, 0x02, 0x5e, 0x02,
    0x03, 0x04, 0x05, 0x06, 0xc0, 0xa8, 0x16, 0x03,
    0x52, 0x54, 0x00, 0x12, 0x34, 0x56, 0xc0, 0xa8,
    0x16, 0x02
  };
  char syn_packet_bytes[] = {
    0x5e, 0x02, 0x03, 0x04, 0x05, 0x06, 0x52, 0x54,
    0x00, 0x12, 0x34, 0x56, 0x08, 0x00, 0x45, 0x00,
    0x00, 0x28, 0x00, 0x00, 0x00, 0x00, 0x64, 0x06,
    0xa9, 0x7a, 0xc0, 0xa8, 0x16, 0x02, 0xc0, 0xa8,
    0x16, 0x03, 0x4e, 0x20, 0x07, 0xd3, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0x02,
    0x10, 0x00, 0x9c, 0x99, 0x00, 0x00
  };
  char syn_ack_packet_bytes[] = {
    0x52, 0x54, 0x00, 0x12, 0x34, 0x56, 0x5e, 0x02,
    0x03, 0x04, 0x05, 0x06, 0x08, 0x00, 0x45, 0x00,
    0x00, 0x2c, 0x00, 0x00, 0x40, 0x00, 0x40, 0x06,
    0x8d, 0x76, 0xc0, 0xa8, 0x16, 0x03, 0xc0, 0xa8,
    0x16, 0x02, 0x07, 0xd3, 0x4e, 0x20, 0xc9, 0xa0,
    0x34, 0x2e, 0x00, 0x00, 0x00, 0x01, 0x60, 0x12,
    0xfa, 0xf0, 0x9c, 0x0c, 0x00, 0x00, 0x02, 0x04,
    0x05, 0xb4
  };
  char ack_packet_bytes[] = {
    0x5e, 0x02, 0x03, 0x04, 0x05, 0x06, 0x52, 0x54,
    0x00, 0x12, 0x34, 0x56, 0x08, 0x00, 0x45, 0x00,
    0x00, 0x28, 0x00, 0x00, 0x00, 0x00, 0x64, 0x06,
    0xa9, 0x7a, 0xc0, 0xa8, 0x16, 0x02, 0xc0, 0xa8,
    0x16, 0x03, 0x4e, 0x20, 0x07, 0xd3, 0x00, 0x00,
    0x00, 0x01, 0xc9, 0xa0, 0x34, 0x2f, 0x50, 0x10,
    0x10, 0x00, 0x9e, 0xba, 0x00, 0x00
  };

  // arp request
  struct mbuf *arp_request = mbufq_pophead(&tx_queue);
  if (memcmp(arp_request->head, arp_request_packet_bytes, sizeof(arp_request_packet_bytes)) != 0) {
    panic("not match arp_request");
  }
  mbuffree(arp_request);

  // arp reply
  struct mbuf *arp_reply = create_packet(arp_reply_packet_bytes, sizeof(arp_reply_packet_bytes));
  nic_mock_recv(arp_reply);

  // syn packet
  struct mbuf *syn = mbufq_pophead(&tx_queue);
  if (memcmp(syn->head, syn_packet_bytes, syn->len) != 0) {
    panic("not match syn");
  }
  mbuffree(syn);

  // syn ack packet
  struct mbuf *syn_ack = create_packet(syn_ack_packet_bytes, sizeof(syn_ack_packet_bytes));
  nic_mock_recv(syn_ack);

  // ack
  struct mbuf *ack = mbufq_pophead(&tx_queue);
  if (memcmp(ack->head, ack_packet_bytes, ack->len) != 0) {
    panic("not match syn");
  }
  mbuffree(ack);

  // no packet transmit
  if (mbufq_pophead(&tx_queue) != 0) {
    panic("why transmit a packet??");
  }

  if (scb->state != SOCK_CB_ESTAB) {
    panic("why scb is not ESTAB?");
  }
  free_sock_cb(scb);
  printf("\t\t[connect_handshake test] done...!\n");
}
 