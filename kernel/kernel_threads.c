#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

struct proc* allocproc(void);

void
ping(void)
{
  release(&myproc()->lock);
  intr_on();
  for (;;) {
    printf("running proc %d on hart %d\n", myproc()->pid, cpuid());

    // spin for a while to reduce the spam
    for (int i = 0; i < 5000000000; i++)
        continue;
  }
}

// set up our test threads
void
kthreadsinit(void)
{
  struct proc *ping_proc;

  for (int i = 0; i < 30; i++) {
    ping_proc = allocproc();
    safestrcpy(ping_proc->name, "ping", sizeof(ping_proc->name));
    ping_proc->state = RUNNABLE;
    ping_proc->priority = i;
    release(&ping_proc->lock);
    ping_proc->context.ra = (uint64)ping;
  }
}

