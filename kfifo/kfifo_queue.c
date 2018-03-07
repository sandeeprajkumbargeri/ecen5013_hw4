/*  Author: Sandeep Raj Kumbargeri <sandeep.kumbargeri@colorado.edu>
    Date: 6-March-2018
    Description: This kernel module uses the kthread API to create a kernel module
                 with a second thread that allows the two to communicate via queues
                 (kfifo). The first thread should send information to the second thread
                 on a timed interval through the fifo. The second thread should take
                 data from the kfifo and print it to the kernel logger. The info you
                 should pass should relate to the currently scheduled processes in the
                 rbtree. What was the ID and vruntime of the previous, current, and next PID.

    To Build:    sudo make
    To Run:      sudo insmod ./kfifo_queue.ko
    Output using:dmesg
    Remove:      sudo rmmod kfifo_queue

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
#include <linux/string.h>
#include <linux/kfifo.h>
#include <linux/kthread.h>
#include <linux/interrupt.h>
#include <linux/err.h>
#include <linux/irq.h>
#include <linux/clk.h>
#include <linux/list.h>
#include <linux/rtmutex.h>
#include <linux/hrtimer.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/proc_fs.h>


//Kernel module specifics
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sandeep Raj Kumbargeri");
MODULE_DESCRIPTION("This is a kernel module that implements the KFIFO communication between two kernel threads.\n");
MODULE_VERSION("1.0");

#define INTERVAL_MS  1000
#define FIFO_LENGTH  64

//Declaration functions
static int __init init_kfifo_queue(void);
void timer_callback_handler(unsigned long);
int worker_thread(void *);
int logger_thread(void *);
static void __exit exit_kfifo_queue(void);

struct payload
{
  unsigned int pid;
  unsigned long long int vruntime;
};

static DECLARE_KFIFO(kfifo, struct payload, FIFO_LENGTH);
static struct timer_list timer;
static struct semaphore sem;
static struct task_struct *worker;
static struct task_struct *logger;

static int __init init_kfifo_queue(void)
{
  printk(KERN_INFO "## KFIFO INIT ## Starting kfifo_queue module.\n");

  //Setting up timer for regular execution of a thread
  init_timer(&timer);
  setup_timer(&timer, timer_callback_handler, 0);
  mod_timer(&timer, jiffies + msecs_to_jiffies(INTERVAL_MS));
  sema_init(&sem, 0);       //to synchnorize the working of the worker thread

  worker = kthread_create(worker_thread, NULL, "worker thread");
  wake_up_process(worker);

  return 0;
}

//Kernel timer interrupt handler - whenever the timer expires, post the semaphore
void timer_callback_handler(unsigned long data)
{
  mod_timer(&timer, jiffies + msecs_to_jiffies(INTERVAL_MS));
  up(&sem);
}

int worker_thread(void *data)
{
  struct task_struct *current_task = current;
  struct task_struct *previous_task;
  struct task_struct *next_task;
  struct payload previous_task_msg, current_task_msg, next_task_msg;

  INIT_KFIFO(kfifo);          //Initializing the KFIFO

  while(1)
  {
    down(&sem);       //ececute the following code when the semaphore is obrained

    previous_task = list_entry(current_task->tasks.prev, struct task_struct, tasks);

    //next task will only make sense if another process is called after inserting this kernel module
    next_task = list_entry(current_task->tasks.next, struct task_struct, tasks);

    previous_task_msg.pid = previous_task->pid;
    previous_task_msg.vruntime = previous_task->se.vruntime;
    //printk(KERN_INFO "## WORKER ## PREVIOUS TASK PID: %d | VRUNTIME: %llu ##\n", previous_task_msg.pid, previous_task_msg.vruntime);

    current_task_msg.pid = current_task->pid;
    current_task_msg.vruntime = current_task->se.vruntime;
    //printk(KERN_INFO "## WORKER ## CURRENT TASK PID: %d | VRUNTIME: %llu ##\n", current_task_msg.pid, current_task_msg.vruntime);

    next_task_msg.pid = next_task->pid;
    next_task_msg.vruntime = next_task->se.vruntime;
    //printk(KERN_INFO "## WORKER ## NEXT TASK PID: %d | VRUNTIME: %llu ##\n", next_task_msg.pid, next_task_msg.vruntime);

    //Piping the data unit by unit through the FIFO
    kfifo_put(&kfifo, previous_task_msg);
    kfifo_put(&kfifo, current_task_msg);
    kfifo_put(&kfifo, next_task_msg);

    logger = kthread_create(logger_thread, NULL, "logger thread");  //creating the logger thread
    wake_up_process(logger);
  }
  return 0;
}

int logger_thread(void *data)
{
  struct payload message;
  message.pid = 0;
  message.vruntime = 0;

  kfifo_get(&kfifo, &message);
  printk(KERN_INFO "## LOGGER ## PREVIOUS TASK PID: %d | VRUNTIME: %llu ##\n", message.pid, message.vruntime);

  kfifo_get(&kfifo, &message);
  printk(KERN_INFO "## LOGGER ## CURRENT TASK PID: %d | VRUNTIME: %llu ##\n", message.pid, message.vruntime);

  kfifo_get(&kfifo, &message);
  printk(KERN_INFO "## LOGGER ## NEXT TASK PID: %d | VRUNTIME: %llu ##\n", message.pid, message.vruntime);

  return 0;
}

static void __exit exit_kfifo_queue(void)
{
  //clean up by removing the kernel timer and stopping the worker thread
   del_timer(&timer);
   kthread_stop(worker);
   printk(KERN_INFO "## KFIFO EXIT ## Exiting kfifo_queue module.\n");
}

//Letting the kernel module know baout the init and the exit functions
module_init(init_kfifo_queue);
module_exit(exit_kfifo_queue);
