// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"
#include "proc.h"

int freerange(void *pa_start, void *pa_end);
void* initrefs(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

int debug;
struct {
  struct spinlock lock;
  struct spinlock reflock;
  struct run *freelist;
  char *refcounts;
  uint64 start;
  int pgcount;
} kmem;

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  initlock(&kmem.reflock, "kmemref");
  freerange(initrefs(end, (void*)PHYSTOP), (void*)PHYSTOP);
}

void*
initrefs(void *pa_start, void *pa_end)
{
  int count = ((char*)pa_end - (char*)PGROUNDUP((uint64)pa_start))/PGSIZE;
  kmem.refcounts = pa_start;
  kmem.pgcount = count;
  memset(kmem.refcounts, 0, count);
  return (void*)((char*)pa_start+count);
}

int
freerange(void *pa_start, void *pa_end)
{
  char *p;
  int pages = 0;
  p = (char*)PGROUNDUP((uint64)pa_start);
  kmem.start = (uint64)p;
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE) {
    kfree(p);
    pages++;
  }
  return pages;
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;
  int refs;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");
  refs = kgetref((uint64)pa);
  if (debug)
    printf("kfree %d %d by %s\n", PG_IDX((uint64)pa, kmem.start), refs, myproc()->name);
  if (refs < 0) {
    panic("kfree");
  } else if (refs > 0) {
    return;
  }
  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
    kmem.freelist = r->next;
  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}

void
krefincr(uint64 pa)
{
  int idx = PG_IDX(pa, kmem.start);
  if (idx >= kmem.pgcount || idx < 0) {
    panic("krefincr");
  }

  acquire(&kmem.reflock);
  ++kmem.refcounts[idx];
  release(&kmem.reflock);
  if (debug)
    printf("krefincr %d %d by %s\n", idx, kmem.refcounts[idx], myproc()->name);
}

void
krefdecr(uint64 pa)
{
  //pa = PGROUNDDOWN(pa);
  int idx = PG_IDX(pa, kmem.start);
  if (idx >= kmem.pgcount || idx < 0) {
    panic("krefdecr");
  }
  if (kmem.refcounts[idx] == 0) {

    printf("pa=%p            sepc=%p stval=%p\n",pa, r_sepc(), r_stval());
    panic("krefdecr");
  }
  acquire(&kmem.reflock);
  --kmem.refcounts[idx];
  release(&kmem.reflock);
  if (debug)
    printf("krefdecr %d %d by %s\n", idx, kmem.refcounts[idx], myproc()->name);
}

int
kgetref(uint64 pa)
{
  int idx = PG_IDX(pa, kmem.start);
  if (idx >= kmem.pgcount || idx < 0) {
    panic("kgetref");
  }
  acquire(&kmem.reflock);
  int res = kmem.refcounts[idx];
  release(&kmem.reflock);
  return res;
}
