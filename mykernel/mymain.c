/*
*  linux/mykernel/mymain.c
*
*  Kernel internal my_start_kernel
*
*  Copyright (C) 2013  Mengning
*
*/
#include <linux/types.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/stackprotector.h>
#include <linux/string.h>
#include <linux/ctype.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <linux/init.h>
#include <linux/initrd.h>
#include <linux/bootmem.h>
#include <linux/acpi.h>
#include <linux/tty.h>
#include <linux/percpu.h>
#include <linux/kmod.h>
#include <linux/vmalloc.h>
#include <linux/kernel_stat.h>
#include <linux/start_kernel.h>
#include <linux/security.h>
#include <linux/smp.h>
#include <linux/profile.h>
#include <linux/rcupdate.h>
#include <linux/moduleparam.h>
#include <linux/kallsyms.h>
#include <linux/writeback.h>
#include <linux/cpu.h>
#include <linux/cpuset.h>
#include <linux/cgroup.h>
#include <linux/efi.h>
#include <linux/tick.h>
#include <linux/interrupt.h>
#include <linux/taskstats_kern.h>
#include <linux/delayacct.h>
#include <linux/unistd.h>
#include <linux/rmap.h>
#include <linux/mempolicy.h>
#include <linux/key.h>
#include <linux/buffer_head.h>
#include <linux/page_cgroup.h>
#include <linux/debug_locks.h>
#include <linux/debugobjects.h>
#include <linux/lockdep.h>
#include <linux/kmemleak.h>
#include <linux/pid_namespace.h>
#include <linux/device.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/signal.h>
#include <linux/idr.h>
#include <linux/kgdb.h>
#include <linux/ftrace.h>
#include <linux/async.h>
#include <linux/kmemcheck.h>
#include <linux/sfi.h>
#include <linux/shmem_fs.h>
#include <linux/slab.h>
#include <linux/perf_event.h>
#include <linux/file.h>
#include <linux/ptrace.h>
#include <linux/blkdev.h>
#include <linux/elevator.h>
#include <asm/io.h>
#include <asm/bugs.h>
#include <asm/setup.h>
#include <asm/sections.h>
#include <asm/cacheflush.h>
#ifdef CONFIG_X86_LOCAL_APIC
#include <asm/smp.h>
#endif
#include "pcb.h"
PCB task[MAX_TASK_NUM];
PCB *my_current_task = NULL;
volatile int need_sched = 0;
void my_process(void);

/* call my_start_kernel before rest_init() in the very beginning. */
void __init my_start_kernel(void)
{
    int pid = 0;
    int i = 0;
    task[pid].pid = pid;
    task[pid].stat = 0;             /* -1 unrunnable, 0 runnable, >0 stopped */
    task[pid].task_entry = task[pid].thread.eip = (unsigned long)my_process;
    task[pid].thread.esp = (unsigned long)&task[pid].stack[KERNEL_STACK_SIZE-1];
    task[pid].next = &task[pid];

    for(i = 1; i < MAX_TASK_NUM; i++) {
        memcpy(&task[i], &task[0], sizeof(PCB));
        task[i].pid = pid;
        task[i].stat = -1;
        task[i].thread.esp = (unsigned long)&task[pid].stack[KERNEL_STACK_SIZE-1];
        task[i-1].next = &task[i];
    }
    
    /*start process 0 */
    pid = 0;
    my_current_task = &task[pid];
    asm volatile (
        "movl %1, %%esp\n\t"    /* esp = task[pid].thread.esp */
        "pushl %1\n\t"          /* stack is empty, so ebp equals esp. push ebp here */
        "pushl %0\n\t"          /* push task[pid].thread.eip */
        "ret\n\t"               /* pop task[pid].thread.eip to eip */
        "popl %%ebp\n\t"        /* pop ebp to ebp */
        :
        :"c" (task[pid].thread.eip),"d" (task[pid].thread.esp) /* c or d mean %ecx or %edx*/
    );

}

void my_process(void)
{
    int i = 0;
    while (1) {
        i++;
        if (i%100000 == 0) {
            printk(KERN_NOTICE "this is process %d\n", my_current_task->pid);
            if(need_sched == 1) {
                /* need_sched will be enabled by timer_interrupt() automatically. */
                need_sched = 0;
                my_schedule();
            }
            printk(KERN_NOTICE "this is process %d\n", my_current_task->pid);
        }
    }

    return;
}
