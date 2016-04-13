/*************************************************************************
  > File Name: pcb.h
  > Author: 
  > Mail: 
  > Created Time: 2016年04月12日 星期二 09时58分51秒
 ************************************************************************/
#ifndef _PCB_H
#define _PCB_H
#endif
#define MAX_TASK_NUM 4
#define KERNEL_STACK_SIZE 1024*8

struct Thread {
    unsigned long eip;
    unsigned long esp;
};

typedef struct pcb {
    int pid;
    volatile unsigned int stat;            /* -1: unrunnable, 0: runnable, >0: stopped */
    char stack[KERNEL_STACK_SIZE];
    struct Thread thread;
    unsigned long task_entry;
    struct pcb *next;
} PCB;

void my_schedule(void);
