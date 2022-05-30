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

#define NBUCKET 10

struct {
  struct spinlock lock;
  struct buf buf[NBUF];
} bcache;

struct bucket {
  struct spinlock lock;
  struct buf head;
} hashtable[NBUCKET];

int hash(uint blockno) {
  return blockno % NBUCKET;
}

void
binit(void)
{
  struct buf *b;

  initlock(&bcache.lock, "bcache");

  for (b = bcache.buf; b < bcache.buf + NBUF; b++) {
    initsleeplock(&b->lock, "buffer");
  }
  b = bcache.buf;
  for (int i = 0; i < NBUCKET; i++) {
    initlock(&hashtable[i].lock, "bcache_bucket");
    for (int j = 0; j < NBUF / NBUCKET; j++) {
      b->blockno = i;
      b->next = hashtable[i].head.next;
      hashtable[i].head.next = b;
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

  int idx = hash(blockno);
  struct bucket* bucket = &hashtable[idx];
  acquire(&bucket->lock);

  // already cached
  for (b = bucket->head.next; b != 0; b = b->next) {
    if (b->dev == dev && b->blockno == blockno) {
      b->refcnt++;
      release(&bucket->lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // LRU
  int min_time = 0x8fffffff;
  struct buf *victim = 0;
  for(b = bucket->head.next; b !=0; b = b->next) {
    if (b->refcnt == 0 && b->timestamp < min_time) {
      victim = b;
      min_time = b->timestamp;
    }
  }
  
  if (!victim) {
    acquire(&bcache.lock);
    refind:
    for(b = bcache.buf; b < bcache.buf + NBUF; b++) {
      if (b->refcnt == 0 && b->timestamp < min_time) {
        victim = b;
        min_time = b->timestamp;
      }
    }
    if (!victim) {
      panic("bget: no buffers");
    } else {
      int victim_idx = hash(victim->blockno);
      acquire(&hashtable[victim_idx].lock);
      if (victim->refcnt != 0) {
        release(&hashtable[victim_idx].lock);
        goto refind;
      }
      struct buf* prev = &hashtable[victim_idx].head;
      struct buf* p = hashtable[victim_idx].head.next;
      while (p != victim) {
        prev = prev->next;
        p = p->next;
      }
      prev->next = p->next;
      release(&hashtable[victim_idx].lock);
      victim->next = hashtable[idx].head.next;
      hashtable[idx].head.next = victim;
      release(&bcache.lock);
    }
  }
  
  victim->dev = dev;
  victim->blockno = blockno;
  victim->valid = 0;
  victim->refcnt = 1;
  release(&bucket->lock);
  acquiresleep(&victim->lock);
  return victim;
  // struct buf *b;

  // acquire(&bcache.lock);

  // // Is the block already cached?
  // for(b = bcache.head.next; b != &bcache.head; b = b->next){
  //   if(b->dev == dev && b->blockno == blockno){
  //     b->refcnt++;
  //     release(&bcache.lock);
  //     acquiresleep(&b->lock);
  //     return b;
  //   }
  // }

  // // Not cached.
  // // Recycle the least recently used (LRU) unused buffer.
  // for(b = bcache.head.prev; b != &bcache.head; b = b->prev){
  //   if(b->refcnt == 0) {
  //     b->dev = dev;
  //     b->blockno = blockno;
  //     b->valid = 0;
  //     b->refcnt = 1;
  //     release(&bcache.lock);
  //     acquiresleep(&b->lock);
  //     return b;
  //   }
  // }
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
  
  int idx = hash(b->blockno);
  acquire(&hashtable[idx].lock);
  b->refcnt --;
  if (b->refcnt == 0) {
    b->timestamp = ticks;
  }
  release(&hashtable[idx].lock);
}

void
bpin(struct buf *b) {
  int idx = hash(b->blockno);
  acquire(&hashtable[idx].lock);
  b->refcnt++;
  release(&hashtable[idx].lock);
}

void
bunpin(struct buf *b) {
  int idx = hash(b->blockno);
  acquire(&hashtable[idx].lock);
  b->refcnt--;
  release(&hashtable[idx].lock);
}