#include "sysinfo.h"
#include "defs.h"
#include "proc.h"
#include "types.h"

int
systeminfo(uint64 info) {
    struct proc* p = myproc();
    struct sysinfo local_info;

    local_info.freemem = freemem();
    local_info.nproc = nproc();

    if (copyout(p->pagetable, info, (char*)&local_info, sizeof(local_info)) < 0) {
      return -1;
    }
    return 0;
}