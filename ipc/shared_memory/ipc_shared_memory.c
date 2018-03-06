/*  Author: Sandeep Raj Kumbargeri <sandeep.kumbargeri@colorado.edu>
    Date: 5-March-2018
    Description: A program to demonstrate the implementation of POSIX Shared Memory IPC mechanishm in Linux.
                 The parent ptocess creates a child process and communicates a structure using shared memory.
                 The child reads the data, modifies it and updates the data back to the parent process through the same shared memory.

    To Build:    gcc -o ipc_shared_memory ipc_shared_memory.c -lrt -lpthread

    Written for ECEN 5013 at University of Colorado Boulder in Spring 2018.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <stdbool.h>

//Structure of the data which is communicated between the parent and the child using pipes.
typedef struct payload
{
  char string[16];
  bool led_state;
} payload_t;

void errExit(char *);

int main(void)
{
  pid_t Child_Pid = 0;
  int shm;
  sem_t *sem;
  void *map_addr = NULL;
  payload_t data;

  bzero(&data, sizeof(payload_t));
  shm_unlink("shared_memory");
  sem_unlink("semaphore");

  printf("## PARENT ## Forking child process.\n");

  switch (Child_Pid = fork())
  {
    case -1: /* fork() failed */
      errExit("fork");
      break;

    case 0: /* Child of successful fork() comes here */
      printf("## CHILD ## Forked.\n");
      shm = shm_open("shared_memory", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
      sem = sem_open("semaphore", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, 0);

      if(shm == -1)
        errExit("client side creation of shared memory descriptor");

      if(ftruncate(shm, sizeof(payload_t)) == -1)
        errExit("fruncate");

      map_addr = mmap(NULL, sizeof(payload_t), PROT_READ | PROT_WRITE, MAP_SHARED, shm, 0);
      if (map_addr == MAP_FAILED)
        errExit("mmap");

      printf("## CHILD ## Created shared descriptor, set its size to %lu bytes and virtual memory mapped it to the parent process.\n", sizeof(payload_t));

      sem_wait(sem);
      memcpy((void *) &data, map_addr, sizeof(payload_t));           /* Copy shared memory to data*/

      printf("## CHILD ## Received string: \"%s\". Received LED State: %s.\n", data.string, data.led_state ? "true" : "false");

      strcat(data.string, " World");
      data.led_state = !data.led_state;
      printf("## CHILD ## Sending modified string: \"%s\". Modified Received LED State: %s.\n", data.string, data.led_state ? "true" : "false");

      memcpy(map_addr, (void *) &data, sizeof(payload_t));           /* Copy data to shared memory */
      msync(map_addr, sizeof(payload_t), MS_SYNC);
      sem_post(sem);

      close(shm);
      sem_close(sem);
      sem_unlink("semaphore");
      printf("## CHILD ## Communication successful. Unlinked the semaphore.\n");

      break;

    default: /* Parent comes here after successful fork() */
      shm = shm_open("shared_memory", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
      sem = sem_open("semaphore", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, 0);

      if(shm == -1)
        errExit("parent side creation of shared memory descriptor");

      if(ftruncate(shm, sizeof(payload_t)) == -1)
        errExit("fruncate");

      map_addr = mmap(NULL, sizeof(payload_t), PROT_READ | PROT_WRITE, MAP_SHARED, shm, 0);
      if (map_addr == MAP_FAILED)
        errExit("mmap");

      printf("## PARENT ## Created shared descriptor, set its size to %lu bytes and virtual memory mapped it to the parent process.\n", sizeof(payload_t));

      strcpy(data.string, "Hello");
      data.led_state = false;

      printf("## PARENT ## Sleeping for 1 second to make sure child has successfully set up to use shared_memory for IPC.\n");
      usleep(500);

      printf("## PARENT ## Updating string in the shared memory: \"%s\". LED State: %s.\n", data.string, data.led_state ? "true" : "false");

      memcpy(map_addr, (void *) &data, sizeof(payload_t));           /* Copy data to shared memory */
      msync(map_addr, sizeof(payload_t), MS_SYNC);
      sem_post(sem);
      usleep(500);

      bzero(&data, sizeof(payload_t));

      sem_wait(sem);
      memcpy((void *) &data, map_addr, sizeof(payload_t));           /* Copy shared memory to data*/

      printf("## PARENT ## Received string: \"%s\". Received LED State: %s.\n", data.string, data.led_state ? "true" : "false");

      close(shm);
      sem_close(sem);
      sem_unlink("semaphore");
      shm_unlink("shared_memory");
      printf("## PARENT ## Communication successful. Closed and unlinked shared memory and semaphores.\n");

      break;
  }

  exit(EXIT_SUCCESS);

}

//Function to print the error to STDOUT and exit.
void errExit(char *strError)
{
  perror(strError);
  exit(EXIT_FAILURE);
}
