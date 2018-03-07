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

//Kernel module specifics
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sandeep Raj Kumbargeri");
MODULE_DESCRIPTION("This is a kernel module that implements the investigation of the current process tree lineage.\n");
MODULE_VERSION("1.0");

//Declaration of Kernel Init and Exit functions
static int __init init_lineage(void);
static void __exit exit_lineage(void);

static int __init init_lineage(void)
{
  struct task_struct *current_task = current;
  struct list_head *children_tasks_list;
  unsigned int children = 0;

  printk(KERN_INFO "## LINEAGE ## Starting..\n");

  while(current_task->pid != 0)
  {
    printk(KERN_INFO "## LINEAGE ## --------------------------------------------------------------------------------------------------------------------- ##\n");

    /* The following macro traverses through a list of children the current task has*/
    list_for_each(children_tasks_list, &(current_task->children))
    {
        children++;       //Increment the counter for children everytime it goes into the loop
    }

    printk(KERN_INFO "## LINEAGE ## CURRENT PROCESS: \"%s\" | PID: %d | STATE: %ld ## CHILDREN: %u | PRIORITY: %d | NICE VALUE: %d |", current_task->comm, current_task->pid, current_task->state, children, current_task->prio, task_nice(current_task));

    current_task = current_task->parent;
    children = 0;
  }
  return 0;
}

static void __exit exit_lineage(void)
{
  printk(KERN_INFO "## LINEAGE ## --------------------------------------------------------------------------------------------------------------------- ##\n");
  printk(KERN_INFO "## LINEAGE ## Exiting..\n");
}

//Letting the kernel module know baout the init and the exit functions
module_init(init_lineage);
module_exit(exit_lineage);
