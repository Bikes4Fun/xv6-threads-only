#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

struct cpu cpus[NCPU];

struct proc proc[NPROC];

struct proc *initproc;

int nextpid = 1;

struct spinlock pid_lock;


// initialize the proc table.
void
procinit(void)
{
  struct proc *p;
  
  // question: create test processes?
  initlock(&pid_lock, "nextpid");
  for(p = proc; p < &proc[NPROC]; p++) {
      initlock(&p->lock, "proc");
      p->state = UNUSED;
      char *pa = kalloc();
      if(pa == 0)
        panic("kalloc");
      p->kstack = (uint64) pa;
  }
}


// Must be called with interrupts disabled,
// to prevent race with process being moved
// to a different CPU.
int
cpuid()
{
  int id = r_tp();
  return id;
}


// Return this CPU's cpu struct.
// Interrupts must be disabled.
struct cpu*
mycpu(void)
{
  int id = cpuid();
  struct cpu *c = &cpus[id];
  return c;
}


// Return the current struct proc *, or zero if none.
struct proc*
myproc(void)
{
  push_off();
  struct cpu *c = mycpu();
  struct proc *p = c->proc;
  pop_off();
  return p;
}


int
allocpid()
{
  int pid;
  
  acquire(&pid_lock);
  pid = nextpid;
  nextpid = nextpid + 1;
  release(&pid_lock);

  return pid;
}


// Look in the process table for an UNUSED proc.
// If found, initialize state required to run in the kernel,
// and return with p->lock held.
// If there are no free procs, or a memory allocation fails, return 0.
struct proc*
allocproc(void)
{
  struct proc *p;

  for(p = proc; p < &proc[NPROC]; p++) {
    acquire(&p->lock);
    if(p->state == UNUSED) {
      goto found;
    } else {
      release(&p->lock);
    }
  }
  return 0;

found:
  p->pid = allocpid();
  p->state = USED;

  // Set up new context to start executing at forkret,
  // which returns to user space.
  memset(&p->context, 0, sizeof(p->context));
  p->context.sp = p->kstack + PGSIZE;

  return p;
}


// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run.
//  - swtch to start running that process.
//  - eventually that process transfers control
//    via swtch back to the scheduler.
void
scheduler(void)
{
  // proc = [ struct proc *1, struct proc *2 ...]
  // proc: shared between CPU's 
  // struct proc *p; = a pointer to one of the 'struct proc' types
  // from the list of struct procs named 'proc'
  struct proc *p; 
  struct cpu *c = mycpu();
  c->proc = 0;

  unsigned int mul_prime = 2719;
  unsigned int inc_prime = 5813;
  unsigned int SEED = (r_time() + inc_prime) ^ cpuid();
  int random;
  int* ticket_table = kalloc();
  int* time_table = kalloc();

  for(;;){
    int start;
    int end;
    // The most recent process to run may have had interrupts
    // turned off; enable them to avoid a deadlock if all
    // processes are waiting.
    intr_on();

    // fill out a ticket table where the value is the amount of tickets
    // which is accessible at the same index as it's relevant proc?
    int found = 0;
    
    int ticket_sum = 0;
    for (int i = 0; i < NPROC; i++){
      p = &proc[i]; // p = a reference to a pointer to one struct proc item in proc list
      if (p->state != UNUSED && p->state != RUNNING) {
        ticket_table[i] = p->orig_tickets;
        ticket_sum += p->orig_tickets;
        if (p->state == RUNNABLE) {
          found = 1;
        }
      }
    }
 
    if(found == 0) {
      // nothing to run; stop running on this core until an interrupt.
      intr_on();
      asm volatile("wfi");
    }
    else { 
      // draw a random number for tickets
      SEED = (SEED * mul_prime + inc_prime);
      random = SEED % ticket_sum;
      for (int i = 0; i < NPROC; i ++) { 
        int num = random - ticket_table[i];
        if (num <= 0){
          p = &proc[i];
          acquire(&p->lock);
          if (p->state == RUNNABLE) {
            start = r_time();
            p->state = RUNNING; 
            c->proc = p;
            swtch(&c->context, &p->context);
            c->proc = 0;
            end = r_time();
            p->time += end-start;
            time_table[i] = p->time;
          }
          release(&p->lock);
        }
      }
    }
  }
}

// Switch to scheduler.  Must hold only p->lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->noff, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void)
{
  int intena;
  struct proc *p = myproc();

  if(!holding(&p->lock))
    panic("sched p->lock");
  if(mycpu()->noff != 1)
    panic("sched locks");
  if(p->state == RUNNING)
    panic("sched running");
  if(intr_get())
    panic("sched interruptible");

  intena = mycpu()->intena;
  swtch(&p->context, &mycpu()->context);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  struct proc *p = myproc();
  acquire(&p->lock);
  p->state = RUNNABLE;
  sched();
  release(&p->lock);
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();
  
  // Must acquire p->lock in order to
  // change p->state and then call sched.
  // Once we hold p->lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup locks p->lock),
  // so it's okay to release lk.

  acquire(&p->lock);  //DOC: sleeplock1
  release(lk);

  // Go to sleep.
  p->chan = chan;
  p->state = SLEEPING;

  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  release(&p->lock);
  acquire(lk);
}

// Wake up all processes sleeping on chan.
// Must be called without any p->lock.
void
wakeup(void *chan)
{
  struct proc *p;

  for(p = proc; p < &proc[NPROC]; p++) {
    if(p != myproc()){
      acquire(&p->lock);
      if(p->state == SLEEPING && p->chan == chan) {
        p->state = RUNNABLE;
      }
      release(&p->lock);
    }
  }
}



// old scheduler code and notes 
    // set found to one: assume we have not found any runnable processes 
    // until/unless we find one below and then set found to 1
    // for(p = proc; p < &proc[NPROC]; p++) {
    //   acquire(&p->lock);

    //   if(p->state == RUNNABLE) {
    //     // It is the process's job
    //     // to release its lock and then reacquire it
    //     // before jumping back to us.
    //     // start a timer to share time evenly between processes
    //     p->state = RUNNING;
    //     c->proc = p;
    //     // Switch to chosen process.  
    //     // Switch is returned after process finishes? 
    //     swtch(&c->context, &p->context);

    //     // Process is done running for now.
    //     // It should have changed its p->state before coming back.
    //     c->proc = 0;
    //     found = 1;
    //   }
    //   // // deanna: possibly don't need this 
    //   // if (p->current_priority > 0) {
    //   //   p->current_priority -= 1;
    //   // } else {
    //   //   p->current_priority = p->priority;
    //   // }
    //   // // this wil definitely current break the indexing of the ticket table because it's no longer assigned this way:
    //   // ticket_table[p->pid] = p->current_priority; 
    //   // // end of possibly don't need this
      
    //   release(&p->lock); 

    // }
   