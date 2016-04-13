/*
 *  linux/mykernel/myinterrupt.c
 *
 *  Kernel internal my_timer_handler
 *
 *  Copyright (C) 2013  Mengning
 *
 */
#include <linux/kernel_stat.h>
#include <linux/export.h>
#include <linux/interrupt.h>
#include <linux/percpu.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/swap.h>
#include <linux/pid_namespace.h>
#include <linux/notifier.h>
#include <linux/thread_info.h>
#include <linux/time.h>
#include <linux/jiffies.h>
#include <linux/posix-timers.h>
#include <linux/cpu.h>
#include <linux/syscalls.h>
#include <linux/delay.h>
#include <linux/tick.h>
#include <linux/kallsyms.h>
#include <linux/irq_work.h>
#include <linux/sched.h>
#include <linux/sched/sysctl.h>
#include <linux/slab.h>

#include <asm/uaccess.h>
#include <asm/unistd.h>
#include <asm/div64.h>
#include <asm/timex.h>
#include <asm/io.h>

#define CREATE_TRACE_POINTS
#include <trace/events/timer.h>
#include "pcb.h"
extern PCB task[MAX_TASK_NUM];
extern PCB *my_current_task;
extern volatile int need_sched;
volatile int timer_count = 0;

/*
 * Called by timer_interrupt()
 */
void my_timer_handler(void)
{
    if (timer_count % 1000 == 0 && need_sched != 1) {
	    printk(KERN_NOTICE "\n>>>>>>>>>>>>>>>>>my_timer_handler here<<<<<<<<<<<<<<<<<<\n\n");
        need_sched = 1;             /* inform my_schedule to switch process */
    }
    timer_count++;

    return;
}

/* Called by my_process() */
void my_schedule(void) 
{
    PCB *next;
    PCB *prev;

    printk(KERN_NOTICE ">>>>>>>>>>>>>>>>>>SCHEDULE<<<<<<<<<<<<<<<<<<");
    
    /* schedule */
    next = my_current_task->next;
    prev = my_current_task;

    if (next->stat == 0) {
        my_current_task = next;
        printk(KERN_NOTICE ">>>>switch %d to %d ", prev->pid, next->pid);
        
        asm volatile (
            "pushl %%ebp\n\t"               /* save ebp */
            "movl %%esp, %0\n\t"            /* save esp, in this way, ebp will always be right above esp */
            "movl %2, %%esp\n\t"            /* restore esp*/
            "movl $1f, %1\n\t"              /* save eip, the next time the process is called, process will continue at :1 */
            "pushl %3\n\t"                  
            "ret\n\t"                       /* restore eip */
            "1:\t"
            "popl %%ebp\n\t"                /* if ebp is right above esp, popl ebp will be just fine. */
            :"=m" (prev->thread.esp), "=m" (prev->thread.eip)
            : "m" (next->thread.esp), "m" (next->thread.eip)
        );
    } else {
        next->stat = 0;
        my_current_task = next;
        printk(KERN_NOTICE ">>>>switch %d to %d ", prev->pid, next->pid);
        asm volatile (
            "pushl %%ebp\n\t"               /* save ebp */
            "movl %%esp, %0\n\t"            /* save esp */
            "movl %2, %%esp\n\t"            /* restore esp*/
            "movl %2, %%ebp\n\t"            /* new thread, restore ebp*/
            "movl $1f, %1\n\t"                  /* save eip */
            "pushl %3\n\t"                  
            "ret\n\t"                       /* restore eip */
            :"=m"(prev->thread.esp), "=m"(prev->thread.eip)
            : "m"(next->thread.esp), "m"(next->thread.eip)
        );   
    }
    return;
}

