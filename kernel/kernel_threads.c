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
  for (;;) {
    for (int i = 0; i < 100000000; i++);
    printf("running proc %d on hart %d\n", myproc()->pid, cpuid());
    //yield();
    intr_on();
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
    release(&ping_proc->lock);
    ping_proc->context.ra = (uint64)ping;
  }
}

