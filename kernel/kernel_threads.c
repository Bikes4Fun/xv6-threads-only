#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

struct proc* allocproc(void);


void
ping(void){
  // 
  // 
  // 
  release(&myproc()->lock);
  intr_on();
  for (;;) {
    printf("running proc %d on hart %d\nog priority %u, current priority: %u\n", myproc()->pid, cpuid(), myproc()->priority, myproc()->current_priority);
    // spin for a while to reduce the spam
    for (int i = 0; i < 5000000; i++)
        continue;
  }    
}


// set up our test threads
void
kthreadsinit(void)
{
  struct proc *ping_proc;

  for (int i = 0; i < NUMPROC; i++) {
    ping_proc = allocproc();
    safestrcpy(ping_proc->name, "ping", sizeof(ping_proc->name));
    ping_proc->state = RUNNABLE;
    ping_proc->priority = i+1;
    ping_proc->current_priority = i+1;
    release(&ping_proc->lock);
    ping_proc->context.ra = (uint64)ping;
  }
}

