/*  Author: Sandeep Raj Kumbargeri <sandeep.kumbargeri@colorado.edu>
    Date: 6-March-2018
    Description: The kernel module takes the current process's  PID to investigate
                 the process tree lineage. This module should print information
                 about the parent processes as it traverses backwards up the process
                 tree until it cannot go any further. For each process it goes through,
                 it should print the following metrics on that process in dmesg:
                 ○ Thread Name            ○ Process ID        ○ Process Status
                 ○ Number of children     ○ Nice Value        ○ Priority

    To Build:    sudo make
    To Run:      sudo insmod ./klineage.ko
    Output using:dmesg
    Remove:      sudo rmmod klineage

    Written for ECEN 5013 at University of Colorado Boulder in Spring 2018.
*/

//Headers
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/pid.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/timer.h>
#include <linux/semaphore.h>

//Kernel module specifics
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sandeep Raj Kumbargeri");
MODULE_DESCRIPTION("This is a kernel module that implements the KFIFO communication between two kernel threads.\n");
MODULE_VERSION("1.0");

#define INTERVAL_MS  1000
#define FIFO_LENGTH  64

//Declaration functions
static int __init init_module(void);
void timer_callback_handler(unsigned long);
int worker(void *);
int logger(void *);
static void __exit exit_module(void);

typedef struct payload
{
  unsigned int pid;
  unsigned long long int vruntime;
} payload_t;


static DECLARE_KFIFO(kfifo, unsigned int, FIFO_LENGTH);
static struct timer_list timer;
static struct semaphore sem;
static struct task_struct *worker;
static struct task_struct *logger;

static int __init init_module(void)
{
  init_timer(&timer);
  setup_timer(&timer, timer_callback_handler, 0);
  mod_timer(&timer, jiffies + msecs_to_jiffies(INTERVAL_MS));
  sema_init(&sem, 0);

  worker = kthread_create(worker, NULL, "worker thread");
  wake_up_process(worker);

  return 0;
}

//Kernel timer interrupt handler
void timer_callback_handler(unsigned long data)
{
  mod_timer(&timer, jiffies + msecs_to_jiffies(INTERVAL_MS));
  up(&sem);
}

int worker(void *data)
{
  struct task_struct *current_task = current;
  struct task_struct *previous_task;
  struct task_struct *next_task;
  struct list_head *tasks_list;
  payload_t previous_task_msg, current_task_msg, next_task_msg;
  bzero(&message, sizeof(payload_t));

  INIT_KFIFO(kfifo);

  previous_task = list_entry(current_task->tasks.prev, struct task_struct, tasks);
  next_task = list_entry(current_task->tasks.next, struct task_struct, tasks);


}

static void __exit exit_module(void)
{
  printk(KERN_INFO "## LINEAGE ## --------------------------------------------------------------------------------------------------------------------- ##\n");
  printk(KERN_INFO "## LINEAGE ## Exiting..\n");
}

//Letting the kernel module know baout the init and the exit functions
module_init(init_module);
module_exit(exit_module);
