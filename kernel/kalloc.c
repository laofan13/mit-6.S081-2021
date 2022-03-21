// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct Kmem {
  struct spinlock lock;
  struct run* freelist;
  int knum;
  char lock_name[7];
} kmems[NCPU];

void steal_kmem(struct Kmem*, int);

void kinit() {
  for (int i = 0; i < NCPU; i++) {
    snprintf(kmems[i].lock_name, sizeof(kmems[i].lock_name), "kmem_%d", i);
    initlock(&kmems[i].lock, kmems[i].lock_name);
  }
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void kfree(void* pa) {
  struct run* r;

  if (((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  push_off();
  int id = cpuid();

  acquire(&kmems[id].lock);
  r->next = kmems[id].freelist;
  kmems[id].freelist = r;
  kmems[id].knum++;
  release(&kmems[id].lock);

  pop_off();
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void* kalloc(void) {
  struct run* r;
  push_off();
  int id = cpuid();

  struct Kmem* kmem = &kmems[id];

  acquire(&kmem->lock);
  if (kmem->knum == 0) {  //从其他的空闲列表窃取空闲列表
    steal_kmem(kmem, id);
  }

  r = kmem->freelist;
  if (r) {
    kmem->knum--;
    kmem->freelist = r->next;
  }

  release(&kmem->lock);
  pop_off();

  if (r){
    memset((char*)r, 5, PGSIZE);  // fill with junk
  }

  return (void*)r;
}


/* 

    caller must be close interupt push_off and pop_off
 */
void steal_kmem(struct Kmem* kmem, int id) {
  volatile int count;
  for (int i = 0; i < NCPU; i++) {
      if (i == id) continue;

      acquire(&kmems[i].lock);
      struct run *p = kmems[i].freelist;

      if(kmems[i].knum > 0) {
        count = kmems[i].knum >> 1;
        if(count == 0)
          count = 1;

        for (int j = 1; j < count;j++) {
          p = p->next;
        }


        kmem->freelist = kmems[i].freelist;
        kmems[i].freelist = p->next;
        p->next = 0;

        kmems[i].knum -= count;
        kmem->knum = count;

        release(&kmems[i].lock);
        break;
      }
      
      release(&kmems[i].lock);
  }
}
