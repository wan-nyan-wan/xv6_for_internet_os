#include "user.h"
#include "types.h"
#include "arch/riscv.h"
#include "param.h"
#include "styx2000.h"
#include "net/byteorder.h"
#include "fcall.h"

uint8* styx2000_gstring(uint8* p, uint8* ep, char **s) {
  int n;
  if (p + 2 > ep)
    return 0;
  n = GBIT16(p);
  p += 1;
  if (p + n + 1 > ep)
    return 0;
  memmove(p, p+1, n);
  p[n] = '\0';
  *s = (char *)p;
  p += n+1;
  return p;
}

uint8* styx2000_pstring(uint8 *p, char *s) {
	uint32 n;

	if(s == 0){
		PBIT16(p, 0);
		p += BIT16SZ;
		return p;
	}

	n = strlen(s);
	PBIT16(p, n);
	p += BIT16SZ;
	memmove(p, s, n);
	p += n;
	return p;
}

uint16 styx2000_stringsz(char *s) {
	if(s == 0)
		return BIT16SZ;
	return BIT16SZ+strlen(s);
}

uint32 styx2000_getfcallsize(struct styx2000_fcall *f) {
  uint32 n;
	int i;

	n = 0;
	n += BIT32SZ;	/* size */
	n += BIT8SZ;	/* type */
	n += BIT16SZ;	/* tag */

	switch(f->type)
	{
	case STYX2000_TVERSION:
		n += BIT32SZ;
		n += styx2000_stringsz(f->version);
		break;
	case STYX2000_RVERSION:
		n += BIT32SZ;
		n += styx2000_stringsz(f->version);
		break;
	case STYX2000_RERROR:
		n += styx2000_stringsz(f->ename);
		break;
	case STYX2000_TFLUSH:
		n += BIT16SZ;
		break;
	case STYX2000_RFLUSH:
		break;
	case STYX2000_TAUTH:
		n += BIT32SZ;
		n += styx2000_stringsz(f->uname);
		n += styx2000_stringsz(f->aname);
		break;
  case STYX2000_RAUTH:
		n += STYX2000_QIDSZ;
		break;
	case STYX2000_TATTACH:
		n += BIT32SZ;
		n += BIT32SZ;
		n += styx2000_stringsz(f->uname);
		n += styx2000_stringsz(f->aname);
		break;
	case STYX2000_RATTACH:
		n += STYX2000_QIDSZ;
		break;
	case STYX2000_TWALK:
		n += BIT32SZ;
		n += BIT32SZ;
		n += BIT16SZ;
		for(i=0; i<f->nwname; i++)
			n += styx2000_stringsz(f->wname[i]);
		break;
	case STYX2000_RWALK:
		n += BIT16SZ;
		n += f->nwqid*STYX2000_QIDSZ;
		break;
	case STYX2000_TOPEN:
	// case Topenfd:
		n += BIT32SZ;
		n += BIT8SZ;
		break;
	case STYX2000_TCREATE:
		n += BIT32SZ;
		n += styx2000_stringsz(f->name);
		n += BIT32SZ;
		n += BIT8SZ;
		break;
	case STYX2000_ROPEN:
	case STYX2000_RCREATE:
		n += STYX2000_QIDSZ;
		n += BIT32SZ;
		break;
	// case Ropenfd:
	// n += QIDSZ;
	// n += BIT32SZ;
	// n += BIT32SZ;
	// break;
	case STYX2000_TWRITE:
		n += BIT32SZ;
		n += BIT64SZ;
		n += BIT32SZ;
		n += f->count;
		break;
	case STYX2000_RWRITE:
		n += BIT32SZ;
		break;
	case STYX2000_TREAD:
		n += BIT32SZ;
		n += BIT64SZ;
		n += BIT32SZ;
		break;
	case STYX2000_RREAD:
		n += BIT32SZ;
		// n += f->count;
		break;
	case STYX2000_TCLUNK:
	case STYX2000_TREMOVE:
		n += BIT32SZ;
		break;
	case STYX2000_RREMOVE:
		break;
	case STYX2000_RCLUNK:
		break;
	case STYX2000_TSTAT:
		n += BIT32SZ;
		break;
	case STYX2000_RSTAT:
		n += BIT16SZ;
		// n += f->nstat;
		break;
	case STYX2000_TWSTAT:
		n += BIT32SZ;
		n += BIT16SZ;
		n += f->nstat;
		break;
	case STYX2000_RWSTAT:
		break;
	}
	return n;
}

static uint8* parse_hdr(struct styx2000_fcall *fcall, uint8* buf) {
  fcall->size = GBIT32(buf);
  buf += 4;
  fcall->type = GBIT8(buf);
  buf += 1;
  fcall->tag = GBIT16(buf);
  buf += 2;
  return buf;
}

struct styx2000_req* styx2000_parsefcall(uint8* buf, int size) {
  if (buf == 0) {
    return 0;
  }
  if (size < STYX2000_HDR_SIZE) {
    return 0;
  }

  struct styx2000_req *req = styx2000_allocreq();
  if (req == 0) {
    return 0;
  }
  memset(req, 0, sizeof *req);
  struct styx2000_fcall *ifcall = &req->ifcall;

  buf = parse_hdr(ifcall, buf);
  int mlen = ifcall->size - STYX2000_HDR_SIZE;

  switch (ifcall->type) {
    case STYX2000_TVERSION:
      buf = styx2000_parse_tversion(ifcall, buf, mlen);
      break;
    case STYX2000_TAUTH:
      break;
    case STYX2000_TATTACH:
      buf = styx2000_parse_tattach(ifcall, buf, mlen);
      break;
    case STYX2000_TFLUSH:
      break;
    case STYX2000_TWALK:
      break;
    case STYX2000_TOPEN:
      break;
    case STYX2000_TCREATE:
      break;
    case STYX2000_TREAD:
      break;
    case STYX2000_TWRITE:
      break;
    case STYX2000_TCLUNK:
      break;
    case STYX2000_TREMOVE:
      break;
    case STYX2000_TSTAT:
      break;
    case STYX2000_TWSTAT:
      break;
    default:
      goto fail;
  }

  if (buf == 0) {
    goto fail;
  }

  return req;

fail:
  if (req)
    styx2000_freereq(req);
  return 0;
}

int styx2000_composefcall(struct styx2000_req *req, uint8* buf, int size) {
  struct styx2000_fcall *f = &req->ofcall;
  PBIT32(buf, f->size);
  buf += 4;
  PBIT8(buf, f->type);
  buf += 1;
  PBIT16(buf, f->tag);
  buf += 2;

  switch (f->type) {
    case STYX2000_RVERSION:
      if (styx2000_compose_rversion(req, buf) == -1) {
        return -1;
      }
      break;
    case STYX2000_RAUTH:
      return 0;
      break;
    case STYX2000_RATTACH:
      if (styx2000_compose_rattach(req, buf) == -1) {
        return -1;
      }
      break;
    case STYX2000_RERROR:
      break;
    case STYX2000_RFLUSH:
      break;
    case STYX2000_RWALK:
      break;
    case STYX2000_ROPEN:
      break;
    case STYX2000_RCREATE:
      break;
    case STYX2000_RREAD:
      break;
    case STYX2000_RWRITE:
      break;
    case STYX2000_RCLUNK:
      break;
    case STYX2000_RREMOVE:
      break;
    case STYX2000_RSTAT:
      break;
    case STYX2000_RWSTAT:
      break;
    default:
      return -1;
  }
  return 0;
}
