/*
 * physical memory allocator, intended to be used to allocate
 * memory for user processes. allocates in 4096-byte "pages".
 * free list is sorted and combines adjacent pages into
 * long runs, to make it easier to allocate big segments.
 * one reason the page size is 4k is that the x86 segment size
 * granularity is 4k.
 */

#include "param.h"
#include "types.h"
#include "defs.h"
#include "param.h"
#include "mmu.h"
#include "proc.h"
#include "spinlock.h"

struct spinlock kalloc_lock;

struct run {
  struct run *next;
  int len; // bytes
};
struct run *freelist;

/*
 * initialize free list of physical pages. this code
 * cheats by just considering the one megabyte of pages
 * after _end.
 */
void
kinit(void)
{
  extern int end;
  uint mem;
  char *start;

  initlock(&kalloc_lock, "kalloc");
  start = (char*) &end;
  start = (char*) (((uint)start + PAGE) & ~(PAGE-1));
  mem = 256; // assume 256 pages of RAM
  cprintf("mem = %d\n", mem * PAGE);
  kfree(start, mem * PAGE);
}

void
kfree(char *cp, int len)
{
  struct run **rr;
  struct run *p = (struct run*) cp;
  struct run *pend = (struct run*) (cp + len);
  int i;

  if(len % PAGE)
    panic("kfree");

  // XXX fill with junk to help debug
  for(i = 0; i < len; i++)
    cp[i] = 1;

  acquire(&kalloc_lock);

  rr = &freelist;
  while(*rr){
    struct run *rend = (struct run*) ((char*)(*rr) + (*rr)->len);
    if(p >= *rr && p < rend)
      panic("freeing free page");
    if(pend == *rr){
      p->len = len + (*rr)->len;
      p->next = (*rr)->next;
      *rr = p;
      goto out;
    }
    if(pend < *rr){
      p->len = len;
      p->next = *rr;
      *rr = p;
      goto out;
    }
    if(p == rend){
      (*rr)->len += len;
      if((*rr)->next && (*rr)->next == pend){
        (*rr)->len += (*rr)->next->len;
        (*rr)->next = (*rr)->next->next;
      }
      goto out;
    }
    rr = &((*rr)->next);
  }
  p->len = len;
  p->next = 0;
  *rr = p;

 out:
  release(&kalloc_lock);
}

/*
 * allocate n bytes of physical memory.
 * returns a kernel-segment pointer.
 * returns 0 if there's no run that's big enough.
 */
char*
kalloc(int n)
{
  struct run **rr;

  if(n % PAGE)
    panic("kalloc");

  acquire(&kalloc_lock);

  rr = &freelist;
  while(*rr){
    struct run *r = *rr;
    if(r->len == n){
      *rr = r->next;
      release(&kalloc_lock);
      return (char*) r;
    }
    if(r->len > n){
      char *p = (char*)r + (r->len - n);
      r->len -= n;
      release(&kalloc_lock);
      return p;
    }
    rr = &(*rr)->next;
  }
  release(&kalloc_lock);
  cprintf("kalloc: out of memory\n");
  return 0;
}
