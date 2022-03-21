// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

#define NBUCKET 13
#define NBUF 3 * NBUCKET

struct {
  struct spinlock lock;
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
} bcache;

/*
  哈希索引桶
 */
struct Bucket{
  struct spinlock lock;
  struct buf head;
  char lock_name[10];
} hashBucket[NBUCKET];

void binit(void) {
  struct buf *b;
  initlock(&bcache.lock, "bcache");

  // Create linked list of buffers
  b = bcache.buf;
  for (int i = 0; i < NBUCKET; ++i) {
    //init hashBucket item
    snprintf(hashBucket[i].lock_name, sizeof(hashBucket[i].lock_name),"bcache_%d", i);
    initlock(&hashBucket[i].lock, hashBucket[i].lock_name);

    hashBucket[i].head.prev = &hashBucket[i].head;
    hashBucket[i].head.next = &hashBucket[i].head;

    for (int j = 0; j < NBUF / NBUCKET; ++j) {
      initsleeplock(&b->lock, "buffer");
      b->dev = 1;
      b->blockno = i;

      b->next = hashBucket[i].head.next;
      b->prev = &hashBucket[i].head;
      hashBucket[i].head.next->prev = b;
      hashBucket[i].head.next = b;
      b++;
    }
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  struct Bucket* buk;

  // Is the block already cached?
  int hash = blockno % NBUCKET;
  buk = &hashBucket[hash];

  acquire(&buk->lock);
  for(b = buk->head.next; b != &buk->head; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&buk->lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  uint min_timestamp = ~0;
  struct buf* replace_buf = 0;

  for(b = buk->head.next; b != &buk->head; b = b->next){
    if(b->refcnt == 0 && b->timestamp < min_timestamp) {
      replace_buf = b;
      min_timestamp = b->timestamp;
    }
  }
  if(replace_buf) {
    goto find;
  }

  acquire(&bcache.lock);
  for(b = bcache.buf; b != bcache.buf + NBUF; b++){
    if(b->refcnt == 0 && b->timestamp < min_timestamp) {
      replace_buf = b;
      min_timestamp = b->timestamp;
    }
  }

  if(replace_buf) {
    hash = replace_buf->blockno % NBUCKET;

    //steel a buf in other bucket
    acquire(&hashBucket[hash].lock);
    replace_buf->next->prev = replace_buf->prev;
    replace_buf->prev->next = replace_buf->next;
    release(&hashBucket[hash].lock);

    replace_buf->next = buk->head.next;
    replace_buf->prev = &buk->head;
    buk->head.next->prev = replace_buf;
    buk->head.next = replace_buf;
    release(&bcache.lock);
  }else {
    release(&buk->lock);
    release(&bcache.lock);
    panic("bget: no buffers");
  }

find:
  replace_buf->dev = dev;
  replace_buf->blockno = blockno;
  replace_buf->valid = 0;
  replace_buf->refcnt = 1;
  release(&buk->lock);
  acquiresleep(&replace_buf->lock);
  return replace_buf;

  // panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);
  int hash = b->blockno % NBUCKET;
  acquire(&hashBucket[hash].lock);

  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->timestamp = ticks;
  }
  release(&hashBucket[hash].lock);
}

void
bpin(struct buf *b) {
  int hash = b->blockno % NBUCKET;
  acquire(&hashBucket[hash].lock);
  b->refcnt++;
  release(&hashBucket[hash].lock);
}

void
bunpin(struct buf *b) {
  int hash = b->blockno % NBUCKET;
  acquire(&hashBucket[hash].lock);
  b->refcnt--;
  release(&hashBucket[hash].lock);
}


