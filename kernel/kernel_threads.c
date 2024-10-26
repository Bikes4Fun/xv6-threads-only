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
  release(&myproc()->lock);
  intr_on();
  int mypid = myproc()->pid;

  unsigned int tally = 0;
  int color_key = (mypid << 3);
  
  // Clamp color_key to the range [0, 255]
  if (color_key < 10) {
      color_key = 10;
  } else if (color_key > 120) {
      color_key = 120;
  }

  // Calculate RGB values
  int red = 120 + color_key; // This will always be non-negative
  int green = (255 - color_key*2); // Vary green based on process ID
  int blue = 265 - color_key; // Vary blue based on process ID

  for (;;) {
    // Use RGB color mode
    printf("\x1b[38;2;%d;%d;%dm", red, green, blue); // Set foreground color
    // printf("pid %d ; cpu %d: time: %d ", mypid, cpuid(), myproc()->time);
    tally++;
    for (int i = 1; i < tally; i++) {
      printf("|");
    }
    printf("\x1b[0m\n"); // Reset all attributes

    // spin for a while to reduce the spam
    for (int i = 0; i < 30000000; i++)
        continue;
    
  }
}

void
system_check(void) {
    release(&myproc()->lock);
    intr_on();

    unsigned int tally = 0;
    for (;;) {
        // Print a special message for the system check process
        printf("\x1b[38;2;%d;%d;%dm", 0, 0, 255); // Set foreground color
        printf("SYSTEM CHECK - pid %d ; cpu %d: ", myproc()->pid, cpuid());
        tally++;
        for (int i = 1; i < tally; i++) {
            printf("|");
        }
        printf("\x1b[0m\n"); // Reset all attributes

        // Spin for a while to reduce spam
        for (int i = 0; i < 30000000; i++)
            asm volatile("nop");
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
    ping_proc->orig_tickets = i*i;
    release(&ping_proc->lock);
    ping_proc->context.ra = (uint64)ping;
  }
  // custom proc 
  ping_proc = allocproc();
  safestrcpy(ping_proc->name, "ping", sizeof(ping_proc->name));
  ping_proc->state = RUNNABLE;
  ping_proc->orig_tickets = NUMPROC*NUMPROC;
  safestrcpy(ping_proc->name, "system_check", sizeof(ping_proc->name));
  release(&ping_proc->lock);
  ping_proc->context.ra = (uint64)ping;

}

