#include "types.h"
#include "riscv.h"
#include "param.h"
#include "defs.h"
#include "date.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  if(argint(0, &n) < 0)
    return -1;
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  if(argaddr(0, &p) < 0)
    return -1;
  return wait(p);
}

uint64
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;


  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}


#ifdef LAB_PGTBL
int
sys_pgaccess(void)
{
  uint64 start_va;
  int page_count_to_check;
  uint64 bitmask;

  if(argaddr(0, &start_va) < 0 || 
     argint(1, &page_count_to_check) < 0 || 
     argaddr(2, &bitmask) < 0) {
    return -1;
  }
  if (page_count_to_check > sizeof(uint) * 8) {
    return -1;
  }
  if (start_va >= MAXVA) {
    return -1;
  }
  
  uint bitmask_buf = {0}; // 8 * sizeof(uint) bits local buffer
  struct proc* p = myproc();
  
  pagetable_t pagetable;
  for (int i = 0; i < page_count_to_check; i++) {
    pagetable = p->pagetable;
    
    if (start_va >= MAXVA) {
      panic("pgaccess: va outof range");
    }

    for (int level = 2; level > 0; level--) {
      pte_t *pte = &pagetable[PX(level, start_va)];
      if (*pte & PTE_V) {
        pagetable = (pagetable_t)PTE2PA(*pte);
      } else {
        return -1;
      }
    }
    pte_t *pte = &pagetable[PX(0, start_va)];
    if (pte == 0 || (*pte & PTE_V) == 0 || (*pte & PTE_U) == 0) {
      return -1;
    }
    if (*pte & PTE_A) {
      bitmask_buf |= 1L << i;
      *pte = *pte & (~PTE_A);
    }
    start_va += PGSIZE;
  }
  return copyout(p->pagetable, bitmask, (char *)&bitmask_buf, page_count_to_check / 8 + 1);
}
#endif

uint64
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}
