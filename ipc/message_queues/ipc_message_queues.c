/*  Author: Sandeep Raj Kumbargeri <sandeep.kumbargeri@colorado.edu>
    Date: 5-March-2018
    Description: A program to demonstrate the implementation of POSIX message queues in Linux.
                 The parent ptocess creates a child process and communicates a structure using POSIX based message queues.
                 The child receives the data, modifies it and sends the data back to the parent process through the same message queue.

    To Build:    gcc -o ipc_message_queues ipc_message_queues.c -lrt

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
#include <mqueue.h>
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
  mqd_t mq;
  struct mq_attr attr;
  payload_t data;

  bzero(&data, sizeof(payload_t));
  mq_unlink("/message_queue");

  printf("## PARENT ## Forking child process.\n");

  switch (Child_Pid = fork())
  {
    case -1: /* fork() failed */
      errExit("fork");
      break;

    case 0: /* Child of successful fork() comes here */
      printf("## CHILD ## Forked.\n");
      mq = mq_open("/message_queue", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, NULL);

      if(mq == -1)
        errExit("child side creation of message queue descriptor");

      printf("## CHILD ## Created message_queue descriptor.\n");

      if(mq_receive(mq, (char *) &data, sizeof(data), 0) == -1)
        errExit("receiving from parent to child");

      printf("## CHILD ## Received string: \"%s\". Received LED State: %s.\n", data.string, data.led_state ? "true" : "false");

      strcat(data.string, " World");
      data.led_state = !data.led_state;
      printf("## CHILD ## Sending modified string: \"%s\". Modified Received LED State: %s.\n", data.string, data.led_state ? "true" : "false");

      if(mq_send(mq, (const char *) &data, sizeof(data), 0) == -1)
        errExit("sending from child to parent");

      mq_close(mq);
      printf("## CHILD ## Communication successful.\n");

      break;

    default: /* Parent comes here after successful fork() */
      bzero(&attr, sizeof(attr));
      attr.mq_flags = O_RDWR;
      attr.mq_maxmsg = 5;
      attr.mq_msgsize = sizeof(payload_t);

      mq = mq_open("/message_queue", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, &attr);

      if(mq == -1)
        errExit("parent side creation of message queue descriptor");

      printf("## PARENT ## Created message_queue descriptor.\n");

      strcpy(data.string, "Hello");
      data.led_state = false;

      printf("## PARENT ## Sleeping for 1 second to make sure child has successfully set up to use message_queue.\n");
      usleep(500);

      printf("## PARENT ## Sending string: \"%s\". LED State: %s.\n", data.string, data.led_state ? "true" : "false");

      if(mq_send(mq, (const char *) &data, sizeof(data), 0) == -1)
        errExit("sending from parent to child");

      bzero(&data, sizeof(payload_t));

      if(mq_receive(mq, (char *) &data, sizeof(data), 0) == -1)
        errExit("receiving from child to parent");

      printf("## PARENT ## Received string: \"%s\". Received LED State: %s.\n", data.string, data.led_state ? "true" : "false");

      mq_close(mq);
      mq_unlink("/message_queue");
      printf("## PARENT ## Communication successful. Closed and unlinked message queue.\n");

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
