#pragma once

#include "net/tcp_cb.h"

#define TCP_COLLISION_NUM 11
#define TCP_MOD 2 << 31
#define TCP_DEFAULT_WINDOW 65535

#define TCP_FLG_FIN 0x01
#define TCP_FLG_SYN 0x02
#define TCP_FLG_RST 0x04
#define TCP_FLG_PSH 0x08
#define TCP_FLG_ACK 0x10
#define TCP_FLG_URG 0x20

#define TCP_MIN_PORT 25000

#define TCP_HDR_LEN(hdr) (((hdr)->offset >> 4) << 2)
#define TCP_DATA_LEN(hdr, len) ((len)-TCP_HDR_LEN(hdr))
#define TCP_FLG_ISSET(x, y) (((x)&0x3f) & (y))

struct tcp {
  uint16 sport;
  uint16 dport;
  uint32 seq;
  uint32 ack;
// little endian only
// use #define to check endian if you want to support big endian
  uint8 off;
  uint8 flg;
  uint16 wnd;
  uint16 sum;
  uint16 urg;
};

struct tcp_cb *tcp_open(uint32, uint16, uint16, int);
