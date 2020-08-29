#include "user.h"
#include "../p9.h"
#include "net/byteorder.h"

uint8_t* p9_parse_topen(struct p9_fcall *fcall, uint8_t* buf, int len) {
  fcall->fid = GBIT32(buf);
  buf += 4;
  fcall->mode = GBIT8(buf);
  buf += 1;
  return buf;
}

int p9_compose_ropen(struct p9_fcall *f, uint8_t* buf) {
  PBIT8(buf, f->qid.type);
  buf += BIT8SZ;
  PBIT32(buf, f->qid.vers);
  buf += BIT32SZ;
  PBIT64(buf, f->qid.path);
  buf += BIT64SZ;
  PBIT32(buf, f->iounit);
  buf += BIT32SZ;
  return 0;
}