#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "defs.h"

volatile static int started = 0;

void kthreadsinit(void);

// start() jumps here in supervisor mode on all CPUs.
void
main()
{
  if(cpuid() == 0){
    consoleinit();
    printfinit();
    printf("\n");
    printf("xv6 kernel is booting\n");
    printf("\n");
    kinit();         // physical page allocator
    procinit();      // process table
    trapinit();      // trap vectors
    trapinithart();  // install kernel trap vector

    kthreadsinit();  // set up our test threads
    
    __sync_synchronize();
    started = 1;
  } else {
    while(started == 0)
      ;
    __sync_synchronize();
    trapinithart();   // install kernel trap vector
    printf("hart %d starting\n", cpuid());
  }

  scheduler();        
}
